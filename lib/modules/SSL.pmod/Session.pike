#pike __REAL_VERSION__
#pragma strict_types
#require constant(SSL.Cipher)

//! The most important information in a session object is a
//! choice of encryption algorithms and a "master secret" created by
//! keyexchange with a client. Each connection can either do a full key
//! exchange to established a new session, or reuse a previously
//! established session. That is why we have the session abstraction and
//! the session cache. Each session is used by one or more connections, in
//! sequence or simultaneously.
//!
//! It is also possible to change to a new session in the middle of a
//! connection.

import ".";
import Constants;
protected constant Struct = ADT.struct;

#ifdef SSL3_DEBUG
#define SSL3_DEBUG_MSG(X ...)  werror(X)
#else /*! SSL3_DEBUG */
#define SSL3_DEBUG_MSG(X ...)
#endif /* SSL3_DEBUG */

//! Identifies the session to the server
string(8bit) identity;

//! Always COMPRESSION_null.
int compression_algorithm;

//! Constant defining a choice of keyexchange, encryption and mac
//! algorithm.
int cipher_suite;

//! Information about the encryption method derived from the
//! cipher_suite.
Cipher.CipherSpec cipher_spec;

//! 48 byte secret shared between the client and the server. Used for
//! deriving the actual keys.
string(8bit) master_secret;

//! Information about the certificate in use by the peer, such as
//! issuing authority, and verification status.
mapping cert_data;

//! Negotiated protocol version.
ProtocolVersion version;

//! the peer certificate chain
array(string(8bit)) peer_certificate_chain;

//! our certificate chain
array(string(8bit)) certificate_chain;

//! Our private key.
Crypto.Sign.State private_key;

//! The peer's public key (from the certificate).
Crypto.Sign.State peer_public_key;

//! The max fragment size requested by the client.
int max_packet_size = PACKET_MAX_SIZE;

//! Indicates that the packet HMACs should be truncated
//! to the first 10 bytes (80 bits). Cf RFC 3546 3.5.
int(0..1) truncated_hmac;

/*
 * Extensions provided by the peer.
 */

//! RFC 6066 3.1 (SNI)
string(8bit) server_name;

//! The set of <hash, signature> combinations supported by the peer.
//!
//! Only used with TLS 1.2 and later.
//!
//! Defaults to the settings from RFC 5246 7.4.1.4.1.
array(array(int)) signature_algorithms = ({
  // RFC 5246 7.4.1.4.1:
  // Note: this is a change from TLS 1.1 where there are no explicit
  // rules, but as a practical matter one can assume that the peer
  // supports MD5 and SHA-1.
  ({ HASH_sha, SIGNATURE_rsa }),
  ({ HASH_sha, SIGNATURE_dsa }),
  ({ HASH_sha, SIGNATURE_ecdsa }),
});

//! Supported elliptical curve cipher curves in order of preference.
array(int) ecc_curves = ({});

//! The selected elliptical curve point format.
//!
//! @note
//!   May be @expr{-1@} to indicate that there's no supported overlap
//!   between the server and client.
int ecc_point_format = POINT_uncompressed;

//! Negotiated encrypt-then-mac mode.
int encrypt_then_mac = 0;

/*
 * End of extensions.
 */

#if constant(Crypto.ECC.Curve)
//! The ECC curve selected by the key exchange.
//!
//! @int
//!   @value KE_ecdh_ecdsa
//!   @value KE_ecdh_rsa
//!     The curve from the server certificate.
//!
//!   @value KE_ecdhe_ecdsa
//!   @value KE_ecdhe_rsa
//!   @value KE_ecdh_anon
//!     The curve selected for the ECDHE key exchange
//!     (typically the largest curve supported by both
//!     the client and the server).
//! @endint
Crypto.ECC.Curve curve;
#endif /* Crypto.ECC.Curve */

//! Heartbeat mode.
HeartBeatModeType heartbeat_mode = HEARTBEAT_MODE_disabled;

//! Indicates if this session has the required server certificate keys
//! set. No means that no or the wrong type of certificate was sent
//! from the server.
int(0..1) has_required_certificates()
{
  if (!peer_public_key)
    return (cipher_spec->signature_alg == SIGNATURE_anonymous);
  return 1;
}

//! Used to filter certificates not supported by the peer.
//!
//! @param cp
//!   Candidate @[CertificatePair].
//!
//! @param version
//!   Negotiated version of SSL.
//!
//! @param ecc_curves
//!   The set of ecc_curves supported by the peer.
protected int(0..1) is_supported_cert(CertificatePair cp,
				      int ke_mask,
				      int h_max,
				      ProtocolVersion version,
				      array(int) ecc_curves)
{
  // Check if the certificate is useful for any of the
  // key exchange algorithms that the peer supports.
  if (version >= PROTOCOL_TLS_1_2) {
    // In TLS 1.2 and later DH_DSS/DH_RSA and ECDH_ECDSA/ECDH_RSA
    // have been unified, so use the invariant ke_mask.
    // They have been unified, since the signature_algorithms
    // extension allows the peer to specify exactly which
    // combinations it supports, cf below.
    if (!(ke_mask & cp->ke_mask_invariant)) return 0;

    // Check that all sign_algs in the cert chain are supported by the peer.
    foreach(cp->sign_algs, array(int) sign_alg) {
      int found;
      foreach(signature_algorithms, array(int) sup_alg) {
	if (found = equal(sign_alg, sup_alg)) break;
      }
      if (!found) return 0;
    }
  } else if (!(ke_mask & cp->ke_mask))
    return 0;

#if constant(Crypto.ECC.Curve)
  if (cp->key->curve) {
    // Is the ECC curve supported by the client?
    Crypto.ECC.Curve c =
      ([object(Crypto.ECC.SECP_521R1.ECDSA)]cp->key)->curve();
    SSL3_DEBUG_MSG("Curve: %O (%O)\n",
		   c, ECC_NAME_TO_CURVE[c->name()]);
    return has_value(ecc_curves, ECC_NAME_TO_CURVE[c->name()]);
  }
#endif
  return 1;
}

//! Used to filter the set of cipher suites suggested by the peer
//! based on our available certificates.
//!
//! @param suite
//!   Candidate cipher suite.
//!
//! @param ke_mask
//!   The bit mask of the key exchange algorithms supported by
//!   the set of available certificates.
//!
//! @param version
//!   The negotiated version of SSL/TLS.
int(0..1) is_supported_suite(int suite, int ke_mask, ProtocolVersion version)
{
  array(int) suite_info = [array(int)]CIPHER_SUITES[suite];
  if (!suite_info) {
    SSL3_DEBUG_MSG("Suite %s is not supported.\n", fmt_cipher_suite(suite));
    return 0;
  }

  KeyExchangeType ke = [int(0..0)|KeyExchangeType]suite_info[0];
  if (!(ke_mask & (1<<ke))) return 0;

  if (version < PROTOCOL_TLS_1_2) {
    if (sizeof(suite_info) >= 4) {
      // AEAD protocols are not supported prior to TLS 1.2.
      // Variant cipher-suite dependent prfs are not supported prior to TLS 1.2.
      return 0;
    }
    // FIXME: Check hash size >= cert hash size.
  }

  if ((version >= PROTOCOL_TLS_1_1) &&
      (< CIPHER_null, CIPHER_rc4_40, CIPHER_rc2_40, CIPHER_des40 >)
      [suite_info[1]]) {
    // RFC 4346 A.5: Export suites
    // TLS 1.1 implementations MUST NOT negotiate
    // these cipher suites in TLS 1.1 mode.
    // ...
    // TLS 1.1 clients MUST check that the server
    // did not choose one of these cipher suites
    // during the handshake.
    return 0;
  }

  return 1;
}

//! Selects an apropriate certificate, authentication method
//! and cipher suite for the parameters provided by the client.
//!
//! @param certs
//!   The list of @[CertificatePair]s that are applicable to the
//!   @[server_name] of this session.
//!
//! @param client_suites
//!   The set of cipher suites that the client claims to support.
//!
//! @param version
//!   The SSL protocol version to use.
//!
//! Typical client extensions that also are used:
//! @dl
//!   @item @[signature_algorithms]
//!     The set of signature algorithm tuples that
//!     the client claims to support.
//! @enddl
int select_cipher_suite(array(CertificatePair) certs,
			array(int) cipher_suites,
			ProtocolVersion version)
{
  if (!sizeof(cipher_suites)) return 0;

  if (!certs || !sizeof(certs))
  {
    SSL3_DEBUG_MSG("No certificates.\n");
    return 0;
  }

  SSL3_DEBUG_MSG("Candidate certificates: %O\n", certs);

  // Find the set of key exchange and hash algorithms supported by the
  // client.
  int ke_mask = 0;
  int h_max = 0;
  foreach(cipher_suites, int suite) {
    if (CIPHER_SUITES[suite]) {
      ke_mask |= 1 << [int](CIPHER_SUITES[suite][0]);
      Crypto.Hash hash =
	[object(Crypto.Hash)]HASH_lookup[CIPHER_SUITES[suite][2]];
      if (hash && (hash->digest_size() > h_max)) {
	h_max = hash->digest_size();
      }
    }
  }

  // Filter any certs that the client doesn't support.
  certs = [array(CertificatePair)]
    filter(certs, is_supported_cert, ke_mask, h_max, version, ecc_curves);

  if( version<PROTOCOL_TLS_1_2 && sizeof(certs)>1 )
  {
    // GNU-TLS doesn't like eg SHA being used with SHA256 certs.
    // FIXME: Can this be made more narrow?
    array(CertificatePair) c = [array(CertificatePair)]
      filter(certs, lambda(CertificatePair cp)
        {
          Crypto.Hash hash = [object(Crypto.Hash)]
            HASH_lookup[cp->sign_algs[0][0]];
          return hash->digest_size() <= h_max;
        });
    // Don't clear out the entire list though, as that makes all peers
    // fail.
    if( sizeof(c) )
      certs = c;
  }

  SSL3_DEBUG_MSG("Client supported certificates: %O\n", certs);

  // Find the set of key exchange algorithms supported by
  // the remaining certs.
  ke_mask = (1<<KE_null)|(1<<KE_dh_anon)
#if constant(Crypto.ECC.Curve)
    |(1<<KE_ecdh_anon)
#endif
    ;
  if (version >= PROTOCOL_TLS_1_2) {
    ke_mask = `|(ke_mask, @certs->ke_mask_invariant);
  } else {
    ke_mask = `|(ke_mask, @certs->ke_mask);
  }

  // Given the set of certs, filter the set of client_suites,
  // to find the best.
  cipher_suites =
    filter(cipher_suites, is_supported_suite, ke_mask, version);

  if (!sizeof(cipher_suites)) {
    SSL3_DEBUG_MSG("No suites left after certificate filtering.\n");
    return 0;
  }

  SSL3_DEBUG_MSG("intersection:\n%s\n",
                 fmt_cipher_suites(cipher_suites));

  int suite = cipher_suites[0];

  int ke_method = [int]CIPHER_SUITES[suite][0];

  SSL3_DEBUG_MSG("Selecting server key and certificate.\n");

  int max_hash_size = 512;

  // Now we can select the actual cert to use.
  if ( !KE_Anonymous[ke_method] ) {
    CertificatePair cert;

    if (version >= PROTOCOL_TLS_1_2) {
      foreach(certs, CertificatePair cp) {
	if (is_supported_suite(suite, cp->ke_mask_invariant, version)) {
	  cert = cp;
	  break;
	}
      }
    } else {
      foreach(certs, CertificatePair cp) {
	if (is_supported_suite(suite, cp->ke_mask, version)) {
	  cert = cp;
	  break;
	}
      }
    }

    if (!cert) {
      error("No suitable certificate for selected cipher suite: %s.\n",
	    fmt_cipher_suite(suite));
    }

    private_key = cert->key;
    SSL3_DEBUG_MSG("Selected server key: %O\n", private_key);

    certificate_chain = cert->certs;
#if constant(Crypto.ECC.Curve)
    if (private_key->curve) {
      curve = [object(Crypto.ECC.Curve)]private_key->curve();
    }
#endif /* Crypto.ECC.Curve */

    if (private_key->block_size) {
      // FIXME: The maximum allowable hash size depends on the size of the
      //        RSA key when RSA is in use. With a 64 byte (512 bit) key,
      //        the block size is 61 bytes, allow for 23 bytes of overhead.
      max_hash_size = [int]private_key->block_size() - 23;
    }
  }

  if (encrypt_then_mac) {
    // Check if enrypt-then-mac is valid for the suite.
    if (((sizeof(CIPHER_SUITES[suite]) == 3) &&
	 ((< CIPHER_rc4, CIPHER_rc4_40 >)[CIPHER_SUITES[suite][1]])) ||
	((sizeof(CIPHER_SUITES[suite]) == 4) &&
	 (CIPHER_SUITES[suite][3] != MODE_cbc))) {
      // Encrypt-then-MAC not allowed with non-CBC suites.
      encrypt_then_mac = 0;
      SSL3_DEBUG_MSG("Encrypt-then-MAC: Disabled (not valid for suite).\n");
    } else {
      SSL3_DEBUG_MSG("Encrypt-then-MAC: Enabled.\n");
    }
  }

  return set_cipher_suite(suite, version, signature_algorithms,
			  max_hash_size);
}

//! Sets the proper authentication method and cipher specification
//! for the given parameters.
//!
//! @param suite
//!   The cipher suite to use, selected from the set that the client
//!   claims to support.
//!
//! @param version
//!   The SSL protocol version to use.
//!
//! @param signature_algorithms
//!   The set of signature algorithms tuples that the client claims to
//!   support.
//!
//! @param max_hash_size
//!
int set_cipher_suite(int suite, ProtocolVersion version,
		     array(array(int)) signature_algorithms,
		     int max_hash_size)
{
  this::version = version;

  cipher_spec = Cipher.lookup(suite, version, signature_algorithms,
                            truncated_hmac?512:max_hash_size);
  if (!cipher_spec) return 0;

  cipher_suite = suite;
  SSL3_DEBUG_MSG("SSL.Session: cipher_spec %O\n",
                 mkmapping(indices(cipher_spec), values(cipher_spec)));
  return 1;
}

//! Sets the compression method. Currently only @[COMPRESSION_null]
//! and @[COMPRESSION_deflate] are supported.
void set_compression_method(int compr)
{
  if( !(< COMPRESSION_null, COMPRESSION_deflate >)[ compr ] )
    error( "Method not supported\n" );

  compression_algorithm = compr;
}

protected string(8bit) generate_key_block(string(8bit) client_random,
                                          string(8bit) server_random,
                                          ProtocolVersion version)
{
  int required = 2 * (
    cipher_spec->is_exportable ?
    (5 + cipher_spec->hash_size)
    : ( cipher_spec->key_material +
	cipher_spec->hash_size +
	cipher_spec->iv_size)
  );
  string(8bit) key = "";

  key = cipher_spec->prf(master_secret, "key expansion",
			 server_random + client_random, required);

  SSL3_DEBUG_MSG("key_block: %O\n", key);
  return key;
}

#ifdef SSL3_DEBUG
protected void printKey(string name, string key)
{
  werror("%s:  len:%d \t\t%s\n", name, sizeof(key), String.string2hex(key));
}
#endif

//! Generates keys appropriate for the SSL version given in @[version],
//! based on the @[client_random] and @[server_random].
//! @returns
//!   @array
//!     @elem string 0
//!       Client write MAC secret
//!     @elem string 1
//!       Server write MAC secret
//!     @elem string 2
//!       Client write key
//!     @elem string 3
//!       Server write key
//!     @elem string 4
//!       Client write IV
//!     @elem string 5
//!       Server write IV
//!  @endarray
array(string(8bit)) generate_keys(string(8bit) client_random,
                                  string(8bit) server_random,
                                  ProtocolVersion version)
{
  Struct key_data = Struct(generate_key_block(client_random, server_random,
					      version));
  array(string(8bit)) keys = allocate(6);

  SSL3_DEBUG_MSG("client_random: %s\nserver_random: %s\nversion: %d.%d\n",
                 client_random?String.string2hex(client_random):"NULL",
                 server_random?String.string2hex(server_random):"NULL",
                 version>>8, version & 0xff);

  // client_write_MAC_secret
  keys[0] = key_data->get_fix_string(cipher_spec->hash_size);
  // server_write_MAC_secret
  keys[1] = key_data->get_fix_string(cipher_spec->hash_size);

  if (cipher_spec->is_exportable)
  {
    // Exportable (ie weak) crypto.
    if(version == PROTOCOL_SSL_3_0) {
      // SSL 3.0
      keys[2] = Crypto.MD5.hash(key_data->get_fix_string(5) +
				client_random + server_random)
	[..cipher_spec->key_material-1];
      keys[3] = Crypto.MD5.hash(key_data->get_fix_string(5) +
				server_random + client_random)
	[..cipher_spec->key_material-1];
      if (cipher_spec->iv_size)
	{
	  keys[4] = Crypto.MD5.hash(client_random +
				    server_random)[..cipher_spec->iv_size-1];
	  keys[5] = Crypto.MD5.hash(server_random +
				    client_random)[..cipher_spec->iv_size-1];
	}

    } else if(version >= PROTOCOL_TLS_1_0) {
      // TLS 1.0 or later.
      string(8bit) client_wkey = key_data->get_fix_string(5);
      string(8bit) server_wkey = key_data->get_fix_string(5);
      keys[2] = cipher_spec->prf(client_wkey, "client write key",
				 client_random + server_random,
				 cipher_spec->key_material);
      keys[3] = cipher_spec->prf(server_wkey, "server write key",
				 client_random + server_random,
				 cipher_spec->key_material);
      if(cipher_spec->iv_size) {
	string(8bit) iv_block =
	  cipher_spec->prf("", "IV block",
			   client_random + server_random,
			   2 * cipher_spec->iv_size);
	keys[4]=iv_block[..cipher_spec->iv_size-1];
	keys[5]=iv_block[cipher_spec->iv_size..];
	SSL3_DEBUG_MSG("sizeof(keys[4]):%d  sizeof(keys[5]):%d\n",
                       sizeof(keys[4]), sizeof(keys[4]));
      }

    }
    
  }
  else {
    keys[2] = key_data->get_fix_string(cipher_spec->key_material);
    keys[3] = key_data->get_fix_string(cipher_spec->key_material);
    if (cipher_spec->iv_size)
      {
	keys[4] = key_data->get_fix_string(cipher_spec->iv_size);
	keys[5] = key_data->get_fix_string(cipher_spec->iv_size);
      }
  }

#ifdef SSL3_DEBUG
  printKey( "client_write_MAC_secret",keys[0]);
  printKey( "server_write_MAC_secret",keys[1]);
  printKey( "keys[2]",keys[2]);
  printKey( "keys[3]",keys[3]);
  
  if(cipher_spec->iv_size) {
    printKey( "keys[4]",keys[4]);
    printKey( "keys[5]",keys[5]);
    
  } else {
    werror("No IVs!!\n");
  }
#endif
  
  return keys;
}

//! Computes a new set of encryption states, derived from the
//! client_random, server_random and master_secret strings.
//!
//! @returns
//!   @array
//!     @elem SSL.State read_state
//!       Read state
//!     @elem SSL.State write_state
//!       Write state
//!   @endarray
array(State) new_server_states(.Connection con,
				string(8bit) client_random,
				string(8bit) server_random,
				ProtocolVersion version)
{
  State write_state = State(con);
  State read_state = State(con);
  array(string) keys = generate_keys(client_random, server_random, version);

  if (cipher_spec->mac_algorithm)
  {
    read_state->mac = cipher_spec->mac_algorithm(keys[0]);
    write_state->mac = cipher_spec->mac_algorithm(keys[1]);
  }
  if (cipher_spec->bulk_cipher_algorithm)
  {
    read_state->crypt = cipher_spec->bulk_cipher_algorithm();
    read_state->crypt->set_decrypt_key(keys[2]);
    write_state->crypt = cipher_spec->bulk_cipher_algorithm();
    write_state->crypt->set_encrypt_key(keys[3]);
    if (cipher_spec->cipher_type == CIPHER_aead) {
      // AEAD algorithms use other iv methods.
      read_state->tls_iv = write_state->tls_iv = 0;
      read_state->salt = keys[4] || "";
      write_state->salt = keys[5] || "";
    } else if (cipher_spec->iv_size) {
      if (version >= PROTOCOL_TLS_1_1) {
	// TLS 1.1 and later have an explicit IV.
	read_state->tls_iv = write_state->tls_iv = cipher_spec->iv_size;
      }
      read_state->crypt->set_iv(keys[4]);
      write_state->crypt->set_iv(keys[5]);
    }
  }

  switch(compression_algorithm) {
  case COMPRESSION_deflate:
#if constant(Gz)
    read_state->compress = Gz.uncompress;
    write_state->compress = Gz.compress;
#endif
    break;
  }
  return ({ read_state, write_state });
}

//! Computes a new set of encryption states, derived from the
//! client_random, server_random and master_secret strings.
//!
//! @returns
//!   @array
//!     @elem SSL.State read_state
//!       Read state
//!     @elem SSL.State write_state
//!       Write state
//!   @endarray
array(State) new_client_states(.Connection con,
				string(8bit) client_random,
				string(8bit) server_random,
				ProtocolVersion version)
{
  State write_state = State(con);
  State read_state = State(con);
  array(string) keys = generate_keys(client_random, server_random, version);
  
  if (cipher_spec->mac_algorithm)
  {
    read_state->mac = cipher_spec->mac_algorithm(keys[1]);
    write_state->mac = cipher_spec->mac_algorithm(keys[0]);
  }
  if (cipher_spec->bulk_cipher_algorithm)
  {
    read_state->crypt = cipher_spec->bulk_cipher_algorithm();
    read_state->crypt->set_decrypt_key(keys[3]);
    write_state->crypt = cipher_spec->bulk_cipher_algorithm();
    write_state->crypt->set_encrypt_key(keys[2]);
    if (cipher_spec->cipher_type == CIPHER_aead) {
      // AEAD algorithms use other iv methods.
      read_state->tls_iv = write_state->tls_iv = 0;
      read_state->salt = keys[5] || "";
      write_state->salt = keys[4] || "";
    } else if (cipher_spec->iv_size) {
      if (version >= PROTOCOL_TLS_1_1) {
	// TLS 1.1 and later have an explicit IV.
	read_state->tls_iv = write_state->tls_iv = cipher_spec->iv_size;
      }
      read_state->crypt->set_iv(keys[5]);
      write_state->crypt->set_iv(keys[4]);
    }
  }
  return ({ read_state, write_state });
}

//! Returns true if this session object can be used in place of the
//! session object @[other].
int(0..1) reusable_as(Session other)
{
  // SSL3 5.6.1.2:
  // If the session_id field is not empty (implying a session
  // resumption request) this vector [cipher_suites] must
  // include at least the cipher_suite from that session.
  // ...
  // If the session_id field is not empty (implying a session
  // resumption request) this vector [compression_methods]
  // must include at least the compression_method from
  // that session.

  // We use a *much* stricter test, and only reuse the old session
  // if it has the same parameters as the new session.
  return cipher_suite == other->cipher_suite &&
    version == other->version &&
    certificate_chain == other->certificate_chain &&
    compression_algorithm == other->compression_algorithm &&
    max_packet_size == other->max_packet_size &&
    truncated_hmac == other->truncated_hmac &&
    server_name == other->server_name &&
    ecc_point_format == other->ecc_point_format &&
    encrypt_then_mac == other->encrypt_then_mac &&
    equal(signature_algorithms, other->signature_algorithms) &&
    equal(ecc_curves, other->ecc_curves);
}
