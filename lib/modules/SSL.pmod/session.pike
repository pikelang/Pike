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

import .Constants;
protected constant Struct = ADT.struct;

#ifdef SSL3_DEBUG
#define SSL3_DEBUG_MSG(X ...)  werror(X)
#else /*! SSL3_DEBUG */
#define SSL3_DEBUG_MSG(X ...)
#endif /* SSL3_DEBUG */

//! Identifies the session to the server
string(0..255) identity;

//! Always COMPRESSION_null.
int compression_algorithm;

//! Constant defining a choice of keyexchange, encryption and mac
//! algorithm.
int cipher_suite;

//! Information about the encryption method derived from the
//! cipher_suite.
.Cipher.CipherSpec cipher_spec;

//! Key exchange method, also derived from the cipher_suite.
int ke_method;

//! Key exchange factory, derived from @[ke_method].
program(.Cipher.KeyExchange) ke_factory;

//! 48 byte secret shared between the client and the server. Used for
//! deriving the actual keys.
string(0..255) master_secret;

//! information about the certificate in use by the peer, such as issuing authority, and verification status.
mapping cert_data;

array(int) version;

//! the peer certificate chain
array(string(8bit)) peer_certificate_chain;

//! our certificate chain
array(string(8bit)) certificate_chain;

//! Our private key.
Crypto.Sign private_key;

//! The peer's public key (from the certificate).
Crypto.Sign peer_public_key;

/*
 * Extensions provided by the peer.
 */

//! RFC 4366 3.1 (SNI)
array(string(0..255)) server_names;

//! The set of <hash, signature> combinations supported by the other end.
//!
//! Only used with TLS 1.2 and later.
//!
//! Defaults to the settings from RFC 5246 7.4.1.4.1.
array(array(int)) signature_algorithms = ({
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

//! Indicates if this session has the required server certificate keys
//! set. No means that no or the wrong type of certificate was sent
//! from the server.
int(0..1) has_required_certificates()
{
  if (!peer_public_key) return (cipher_spec->sign == .Cipher.anon_sign);
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
				      int version,
				      array(int) ecc_curves)
{
  if (version >= PROTOCOL_TLS_1_2) {
    if (!(ke_mask & cp->ke_mask_invariant)) return 0;

    // FIXME: In TLS 1.2 and later check that all sign_algs
    //        in the cert chain are supported by the peer.
  } else {
    if (!(ke_mask & cp->ke_mask)) return 0;
  }

#if constant(Crypto.ECC.Curve)
  if (cp->key->curve) {
    // Is the ECC curve supported by the client?
    Crypto.ECC.Curve c =
      ([object(Crypto.ECC.SECP_521R1.ECDSA)]cp->key)->curve();
    SSL3_DEBUG_MSG("Curve: %O\n"
		   "CURVE ID: %d\n",
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
protected int(0..1) is_supported_suite(int suite, int ke_mask)
{
  array(int) suite_info = [array(int)]CIPHER_SUITES[suite];
  if (!suite_info) {
    SSL3_DEBUG_MSG("Suite %s is not supported.\n", fmt_cipher_suite(suite));
    return 0;
  }

  KeyExchangeType ke = [int(0..0)|KeyExchangeType]suite_info[0];
  if (ke_mask & (1<<ke)) return 1;
  return 0;
}

//! Selects an apropriate certificate, authentication method
//! and cipher suite for the parameters provided by the client.
//!
//! @param context
//!   The server context.
//!
//! @param client_suites
//!   The set of cipher suites that the client claims to support.
//!
//! @param version
//!   The SSL protocol version to use.
//!
//! Typical client extensions that also are used:
//! @ul
//!   @li @[signature_algorithms]
//!     The set of signature algorithms tuples that
//!     the client claims to support.
//!
//!   @li @[server_names]
//!     Server Name Indication extension from the client.
//!     May be @expr{0@} (zero) if the client hasn't sent any SNI.
//! @endul
int select_cipher_suite(object context,
			array(int) cipher_suites,
			ProtocolVersion|int version)
{
  if (!sizeof(cipher_suites)) return 0;

  // First we need to check what certificate candidates we have.
  array(CertificatePair) certs =
    ([function(array(string(8bit)): array(CertificatePair))]
     context->find_cert)(server_names);

  SSL3_DEBUG_MSG("Candidate certificates: %O\n", certs);

  // Find the set of key exchange algorithms supported by the client.
  int ke_mask = 0;
  foreach(cipher_suites, int suite) {
    if (CIPHER_SUITES[suite]) {
      ke_mask |= 1 << [int](CIPHER_SUITES[suite][0]);
    }
  }

  // Filter any certs that the client doesn't support.
  certs = [array(CertificatePair)]
    filter(certs, is_supported_cert, ke_mask, version, ecc_curves);

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
    ([function(array(int):array(int))]
     context->sort_suites)(filter(cipher_suites, is_supported_suite, ke_mask));

  if (!sizeof(cipher_suites)) {
    SSL3_DEBUG_MSG("No suites left after certificate filtering.\n");
    return 0;
  }

  SSL3_DEBUG_MSG("intersection:\n%s\n",
                 fmt_cipher_suites(cipher_suites));

  int suite = cipher_suites[0];

  ke_method = [int]CIPHER_SUITES[suite][0];

  SSL3_DEBUG_MSG("Selecting server key and certificate.\n");

  int max_hash_size = 512;

  // Now we can select the actual cert to use.
  SignatureAlgorithm sa = [int(0..0)|SignatureAlgorithm]KE_TO_SA[ke_method];
  if (sa != SIGNATURE_anonymous) {
    CertificatePair cert;

    if (version >= PROTOCOL_TLS_1_2) {
      foreach(certs, CertificatePair cp) {
	if (is_supported_suite(suite, cp->ke_mask_invariant)) {
	  cert = cp;
	  break;
	}
      }
    } else {
      foreach(certs, CertificatePair cp) {
	if (is_supported_suite(suite, cp->ke_mask)) {
	  cert = cp;
	  break;
	}
      }
    }

    if (!cert) {
      error("No suitable certificate for selected cipher suite: %s (%s).\n",
	    fmt_cipher_suite(suite), fmt_constant("SIGNATURE_", sa));
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

  return set_cipher_suite(suite, version, signature_algorithms,
			  max_hash_size);
}

//! Sets the proper authentication method and cipher specification
//! for the given parameters.
//!
//! @param client_suites
//!   The set of cipher suites that the client claims to support.
//!
//! @param version
//!   The SSL protocol version to use.
//!
//! @param signature_algorithms
//!   The set of signature algorithms tuples that the client claims to support.
int set_cipher_suite(int suite, ProtocolVersion|int version,
		     array(array(int)) signature_algorithms,
		     int max_hash_size)
{
  array res = .Cipher.lookup(suite, version, signature_algorithms,
			     max_hash_size);
  if (!res) return 0;
  cipher_suite = suite;
  ke_method = [int]res[0];
  switch(ke_method) {
  case KE_null:
    ke_factory = .Cipher.KeyExchangeNULL;
    break;
  case KE_rsa:
    ke_factory = .Cipher.KeyExchangeRSA;
    break;
  case KE_dh_dss:
  case KE_dh_rsa:
    ke_factory = .Cipher.KeyExchangeDH;
    break;
  case KE_dh_anon:
  case KE_dhe_rsa:
  case KE_dhe_dss:
    ke_factory = .Cipher.KeyExchangeDHE;
    break;
#if constant(SSL.Cipher.KeyExchangeECDHE)
  case KE_ecdhe_rsa:
  case KE_ecdhe_ecdsa:
  case KE_ecdh_anon:
    ke_factory = .Cipher.KeyExchangeECDHE;
    break;
  case KE_ecdh_rsa:
  case KE_ecdh_ecdsa:
    ke_factory = .Cipher.KeyExchangeECDH;
    break;
#endif
  default:
    error("set_cipher_suite: Unsupported key exchange method: %d\n",
	  ke_method);
    break;
  }

  cipher_spec = [object(.Cipher.CipherSpec)]res[1];
#ifdef SSL3_DEBUG
  werror("SSL.session: cipher_spec %O\n",
	 mkmapping(indices(cipher_spec), values(cipher_spec)));
#endif
  return 1;
}

//! Sets the compression method. Currently only @[COMPRESSION_null] is
//! supported.
void set_compression_method(int compr)
{
  switch(compr) {
  case COMPRESSION_null:
    break;
  case COMPRESSION_deflate:
    break;
  default:
    error( "Method not supported\n" );
  }
  compression_algorithm = compr;
}

protected string(0..255) generate_key_block(string(0..255) client_random,
					    string(0..255) server_random,
					    array(int) version)
{
  int required = 2 * (
    cipher_spec->is_exportable ?
    (5 + cipher_spec->hash_size)
    : ( cipher_spec->key_material +
	cipher_spec->hash_size +
	cipher_spec->iv_size)
  );
  string(0..255) key = "";

  key = cipher_spec->prf(master_secret, "key expansion",
			 server_random + client_random, required);

#ifdef SSL3_DEBUG
  werror("key_block: %O\n", key);
#endif
  return key;
}

#ifdef SSL3_DEBUG
protected void printKey(string name, string key) {

  string res="";
  res+=sprintf("%s:  len:%d type:%d \t\t",name,sizeof(key),0); 
  /* return; */
  for(int i=0;i<sizeof(key);i++) {
    int d=key[i];
    res+=sprintf("%02x ",d&0xff);
  }
  res+="\n";
  werror(res);
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
array(string(0..255)) generate_keys(string(0..255) client_random,
				    string(0..255) server_random,
				    array(int) version)
{
  Struct key_data = Struct(generate_key_block(client_random, server_random,
					      version));
  array(string(0..255)) keys = allocate(6);

#ifdef SSL3_DEBUG
  werror("client_random: %s\nserver_random: %s\nversion: %d.%d\n",
	 client_random?String.string2hex(client_random):"NULL",
         server_random?String.string2hex(server_random):"NULL",
         version[0], version[1]);
#endif
  // client_write_MAC_secret
  keys[0] = key_data->get_fix_string(cipher_spec->hash_size);
  // server_write_MAC_secret
  keys[1] = key_data->get_fix_string(cipher_spec->hash_size);

  if (cipher_spec->is_exportable)
  {
    // Exportable (ie weak) crypto.
    if(version[1] == PROTOCOL_SSL_3_0) {
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

    } else if(version[1] >= PROTOCOL_TLS_1_0) {
      // TLS 1.0 or later.
      string(0..255) client_wkey = key_data->get_fix_string(5);
      string(0..255) server_wkey = key_data->get_fix_string(5);
      keys[2] = cipher_spec->prf(client_wkey, "client write key",
				 client_random + server_random,
				 cipher_spec->key_material);
      keys[3] = cipher_spec->prf(server_wkey, "server write key",
				 client_random + server_random,
				 cipher_spec->key_material);
      if(cipher_spec->iv_size) {
	string(0..255) iv_block =
	  cipher_spec->prf("", "IV block",
			   client_random + server_random,
			   2 * cipher_spec->iv_size);
	keys[4]=iv_block[..cipher_spec->iv_size-1];
	keys[5]=iv_block[cipher_spec->iv_size..];
#ifdef SSL3_DEBUG
	werror("sizeof(keys[4]):%d  sizeof(keys[5]):%d\n",
	       sizeof(keys[4]), sizeof(keys[4]));
#endif
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
//!     @elem SSL.state read_state
//!       Read state
//!     @elem SSL.state write_state
//!       Write state
//!   @endarray
array(.state) new_server_states(string(0..255) client_random,
				string(0..255) server_random,
				array(int) version)
{
  .state write_state = .state(this);
  .state read_state = .state(this);
  array(string) keys = generate_keys(client_random, server_random,version);

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
    if (cipher_spec->cipher_type == CIPHER_block)
    { // Crypto.Buffer takes care of splitting input into blocks
      read_state->crypt = Crypto.Buffer(read_state->crypt);
      write_state->crypt = Crypto.Buffer(write_state->crypt);
    }
    if (cipher_spec->iv_size)
    {
      if (cipher_spec->cipher_type != CIPHER_aead) {
	if (version[1] >= PROTOCOL_TLS_1_1) {
	  // TLS 1.1 and later have an explicit IV.
	  read_state->tls_iv = write_state->tls_iv = cipher_spec->iv_size;
	}
	read_state->crypt->set_iv(keys[4]);
	write_state->crypt->set_iv(keys[5]);
      } else {
	read_state->tls_iv = write_state->tls_iv = 0;
	read_state->salt = keys[4];
	write_state->salt = keys[5];
      }
    }
  }

  switch(compression_algorithm) {
  case COMPRESSION_deflate:
    // FIXME: RFC 5246 6.2.2:
    //   If the decompression function encounters a TLSCompressed.fragment
    //   that would decompress to a length in excess of 2^14 bytes, it MUST
    //   report a fatal decompression failure error.
    read_state->compress = Gz.inflate()->inflate;
    write_state->compress =
      class(function(string, int:string) _deflate) {
	string deflate(string s) {
	  // RFC 3749 2:
	  //   All data that was submitted for compression MUST be
	  //   included in the compressed output, with no data
	  //   retained to be included in a later output payload.
	  //   Flushing ensures that each compressed packet payload
	  //   can be decompressed completely.
	  return _deflate(s, Gz.SYNC_FLUSH);
	}
      }(Gz.deflate()->deflate)->deflate;
    break;
  }
  return ({ read_state, write_state });
}

//! Computes a new set of encryption states, derived from the
//! client_random, server_random and master_secret strings.
//!
//! @returns
//!   @array
//!     @elem SSL.state read_state
//!       Read state
//!     @elem SSL.state write_state
//!       Write state
//!   @endarray
array(.state) new_client_states(string(0..255) client_random,
				string(0..255) server_random,
				array(int) version)
{
  .state write_state = .state(this);
  .state read_state = .state(this);
  array(string) keys = generate_keys(client_random, server_random,version);
  
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
    if (cipher_spec->cipher_type == CIPHER_block)
    { // Crypto.Buffer takes care of splitting input into blocks
      read_state->crypt = Crypto.Buffer(read_state->crypt);
      write_state->crypt = Crypto.Buffer(write_state->crypt);
    }
    if (cipher_spec->iv_size)
    {
      if (cipher_spec->cipher_type != CIPHER_aead) {
	if (version[1] >= PROTOCOL_TLS_1_1) {
	  // TLS 1.1 and later have an explicit IV.
	  read_state->tls_iv = write_state->tls_iv = cipher_spec->iv_size;
	}
	read_state->crypt->set_iv(keys[5]);
	write_state->crypt->set_iv(keys[4]);
      } else {
	read_state->tls_iv = write_state->tls_iv = 0;
	read_state->salt = keys[5];
	write_state->salt = keys[4];
      }
    }
  }
  return ({ read_state, write_state });
}
