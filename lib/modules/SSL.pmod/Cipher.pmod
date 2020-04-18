#pike __REAL_VERSION__
#require constant(Crypto.Hash)

//! Encryption and MAC algorithms used in SSL.

#include "tls.h"

import .Constants;

// A mapping from DH p to DH q to effort verification level to
// indicate if a DH group with a specific p and q is valid or not. -1
// means failed validation.
protected mapping(Gmp.mpz:mapping(Gmp.mpz:int)) valid_dh;
protected int valid_dh_count;

protected mapping(Gmp.mpz:mapping(Gmp.mpz:int)) make_valid_dh()
{
  mapping dh = ([]);
  foreach(values(Crypto.DH), mixed m)
  {
    if(!objectp(m)) continue;
    object o = [object]m;
    if(m->p && m->q)
      dh[m->p] = ([ m->q : Int.NATIVE_MAX ]);
  }
  return dh;
}

// FIXME: This function ought to check that the selected group
//        is strong enough for the selected key length.
protected bool validate_dh(Crypto.DH.Parameters dh,
			   object session,
			   object context)
{
  if (sizeof(session->ffdhe_groups || ({}))) {
    // Check if one of the recommended groups was selected,
    // in which case we're done.
    foreach(session->ffdhe_groups, int g) {
      Crypto.DH.Parameters ffdhe =
	FFDHE_GROUPS[g] || context->private_ffdhe_groups[g];
      if (!ffdhe) continue;
      if ((ffdhe->p == dh->p) &&
	  (ffdhe->g == dh->g) &&
	  (ffdhe->q == dh->q)) return 1;
    }
    // Also check for the equivalent MODP groups.
    foreach(session->ffdhe_groups, int g) {
      Crypto.DH.Parameters ffdhe = MODP_GROUPS[g];
      if (!ffdhe) continue;
      if ((ffdhe->p == dh->p) &&
	  (ffdhe->g == dh->g) &&
	  (ffdhe->q == dh->q)) return 1;
    }
  }

  if( !valid_dh || valid_dh_count > 1000 )
  {
    valid_dh = make_valid_dh();
    valid_dh_count = 0;
  }

  // Spend more effort validating q when in anonymous KE. These two
  // effort values should possibly be part of the context.
  int effort = 0;
  if( session->cipher_spec->signature_alg == SIGNATURE_anonymous )
    effort = 8; // Misses non-prime with 0.0015% probability.

  mapping pmap = valid_dh[dh->p];
  if( pmap )
  {
    if( has_index(pmap, dh->q) )
      return pmap[dh->q] >= effort;
  }
  else
    valid_dh[dh->p] = ([]);

  if( !dh->validate(effort) )
    effort = -1;

  valid_dh[dh->p][dh->q] = effort;
  valid_dh_count++;
  return effort > -1;
}

//! Cipher algorithm interface.
class CipherAlgorithm {
  this_program set_encrypt_key(string);
  this_program set_decrypt_key(string);
  //! Set the key used for encryption/decryption, and
  //! enter encryption mode.

  int(0..) block_size();
  //! Return the block size for this crypto.

  optional string crypt(string);
  optional string unpad(string,int);
  optional string pad(int);

  optional this_program set_iv(string);
}

//! Message Authentication Code interface.
class MACAlgorithm {

  //! Generates a header and creates a HMAC hash for the given
  //! @[packet].
  //! @param packet
  //!   @[Packet] to generate a MAC hash for.
  //! @param adjust_len
  //!   Added to @tt{sizeof(packet)@} to get the packet length.
  //! @returns
  //!   Returns the MAC hash for the @[packet].
  string hash_packet(object packet, int|void adjust_len);

  //! Creates a HMAC hash of the @[data] with the underlying hash
  //! algorithm.
  string hash(string data);

  //! Creates a normal hash of the @[data] using the underlying hash
  //! algorithm.
  string hash_raw(string data);

  //! The block size of the underlying hash algorithm.
  int block_size();

  //! The length of the header prefixed by @[hash()].
  constant hash_header_size = 13;
}

//! Cipher specification.
class CipherSpec {

  //! Key exchange factory.
  program(KeyExchange) ke_factory;

  //! The algorithm to use for the bulk of the transfered data.
  program(CipherAlgorithm) bulk_cipher_algorithm;

  int cipher_type;

  //! The Message Authentication Code to use for the packets.
  program(MACAlgorithm) mac_algorithm;

  //! Indication whether the combination uses strong or weak
  //! (aka exportable) crypto.
  int is_exportable;

  //! The number of bytes in the MAC hashes.
  int hash_size;

  //! The number of bytes of key material used on initialization.
  int key_material;

  //! The number of bytes of random data needed for initialization vectors.
  int iv_size;

  //! The number of bytes of explicit data needed for initialization vectors.
  //! This is used by AEAD ciphers in TLS 1.2, where there's a secret part of
  //! the iv "salt" of length @[iv_size], and an explicit part that is sent in
  //! the clear.
  //!
  //! This is usually @expr{bulk_cipher_algorithm->iv_size() - iv_size@},
  //! but may be set to zero to just have the sequence number expanded
  //! to the same size as an implicit iv. This is used by the suites
  //! with @[Crypto.ChaCha20.POLY1305].
  int explicit_iv_size;

  //! The effective number of bits in @[key_material].
  //!
  //! This is typically @expr{key_material * 8@}, but for eg @[DES]
  //! this is @expr{key_material * 7@}.
  int key_bits;

  //! The Pseudo Random Function to use.
  //!
  //! @seealso
  //!   @[prf_ssl_3_0()], @[prf_tls_1_0()], @[prf_tls_1_2()]
  function(string(8bit), string(8bit), string(8bit), int:string(8bit)) prf;

  //! The hash algorithm for signing the handshake.
  //!
  //! Usually the same hash as is the base for the @[prf].
  //!
  //! @note
  //!   Only used in TLS 1.2 and later.
  Crypto.Hash hash;

  //! The hash algorithm used for key exchange signatures.
  HashAlgorithm signature_hash = HASH_sha1;

  //! The signature algorithm used for key exchange signatures.
  SignatureAlgorithm signature_alg;

  //! The number of bytes that is safe to send before we must renegotiate the keys.
  int max_bytes;

  //! The function used to sign packets.
  Stdio.Buffer sign(object session, string(8bit) cookie, Stdio.Buffer struct)
  {
    if( signature_alg == SIGNATURE_anonymous )
      return struct;

    string data = cookie + (string)struct;

    // RFC 5246 4.7
    if( session->version >= PROTOCOL_TLS_1_2 )
    {
      string sign =
        session->private_key->pkcs_sign(data, HASH_lookup[signature_hash]);
      struct->add_int(signature_hash|signature_alg, 2);
      struct->add_hstring(sign, 2);
      return struct;
    }

    // RFC 4346 7.4.3 (struct Signature)
    switch( signature_alg )
    {
    case SIGNATURE_rsa:
      {
        string digest = Crypto.MD5->hash(data) + Crypto.SHA1->hash(data);
        struct->add_hint(session->private_key->raw_sign(digest), 2);
        return struct;
      }

    case SIGNATURE_dsa:
    case SIGNATURE_ecdsa:
      {
        string sign = session->private_key->pkcs_sign(data, Crypto.SHA1);
	SSL3_DEBUG_MSG("Signing...\n"
		       "data: %s\n"
		       "Signature: %s\n",
		       String.string2hex(data),
		       String.string2hex(sign));
        struct->add_hstring(sign, 2);
        return struct;
      }
    }

    error("Internal error");
  }

  //! The function used to verify the signature for packets.
  int(0..1) verify(object session, string data, Stdio.Buffer input)
  {
    if( signature_alg == SIGNATURE_anonymous )
      return 1;

    Crypto.Sign.State pkc = session->peer_public_key;
    if( !pkc ) return 0;

    // RFC 5246 4.7
    if( session->version >= PROTOCOL_TLS_1_2 )
    {
      int signature_scheme = input->read_int(2);
      int hash_id = signature_scheme & HASH_MASK;
      int sign_id = signature_scheme & SIGNATURE_MASK;
      string sign = input->read_hstring(2);

      if( sign_id != signature_alg )
      {
        SSL3_DEBUG_MSG("Signature pair <%d,%d> doesn't match "
                       "negotiated <%d,%d>\n", hash_id, sign_id,
                       signature_hash, signature_alg);
        return 0;
      }

      Crypto.Hash hash = HASH_lookup[hash_id];
      if (!hash) return 0;

      return pkc->pkcs_verify(data, hash, sign);
    }

    // RFC 4346 7.4.3 (struct Signature)
    switch( signature_alg )
    {
    case SIGNATURE_rsa:
      {
        string digest = Crypto.MD5->hash(data) + Crypto.SHA1->hash(data);
        // FIXME: We could check that the signature is encoded
        // (padded) to the correct number of bytes
        // (pkc->private_key->key_size()/8).
        Gmp.mpz signature = Gmp.mpz(input->read_hint(2));
        return pkc->raw_verify(digest, signature);
      }

    case SIGNATURE_dsa:
    case SIGNATURE_ecdsa:
      {
	string(8bit) sign = input->read_hstring(2);
	SSL3_DEBUG_MSG("Verifying signature...\n"
		       "Data: %s\n"
		       "Signature: %s\n",
		       String.string2hex(data),
		       String.string2hex(sign));
        return pkc->pkcs_verify(data, Crypto.SHA1, sign);
      }
    }

    error("Internal error");
  }

  void set_hash(int max_hash_size,
                array(int) signature_algorithms)
  {
    // Stay with SHA1 for requests without signature algorithms
    // extensions (RFC 5246 7.4.1.4.1) and anonymous requests.
    if( signature_alg == SIGNATURE_anonymous || !signature_algorithms )
      return;

    int hash_id = -1;
    SSL3_DEBUG_MSG("Signature algorithms (max hash size %d):\n%s",
                   max_hash_size, fmt_signature_pairs(signature_algorithms));
    foreach(signature_algorithms, int pair) {
      if (((pair & SIGNATURE_MASK) == signature_alg) &&
	  HASH_lookup[pair & HASH_MASK]) {
        if (max_hash_size < HASH_lookup[pair & HASH_MASK]->digest_size()) {
          // Eg RSA has a maximum block size and the digest is too large.
          continue;
        }
        if ((pair & HASH_MASK) > hash_id) {
          hash_id = pair & HASH_MASK;
        }
      }
    }

    if (signature_hash == -1)
      error("No acceptable hash algorithm.\n");
    signature_hash = hash_id;

    SSL3_DEBUG_MSG("Selected <%s, %s>\n",
                   fmt_constant(signature_hash, "HASH"),
                   fmt_constant(signature_alg, "SIGNATURE"));
  }
}

//! KeyExchange method base class.
class KeyExchange(object context, object session, object connection,
		  ProtocolVersion client_version)
{
  // 1. Server calls server_key_exchange_packet()
  // 1.1 Unless overloaded server_key_params() is called.
  // 2. Client calls got_server_key_exchange()
  // 2.1 Unless overloaded parse_server_key_exchange() is called.
  // 3. Client calls client_key_exchange_packet()
  // 4. Server calls got_client_key_exchange()

  //! Indicates whether a certificate isn't required.
  int anonymous;

  //! Indicates whether the key exchange has failed due to bad MACs.
  int message_was_bad;

  //! @returns
  //!   Returns an @[Stdio.Buffer] with the
  //!   @[HANDSHAKE_server_key_exchange] payload.
  Stdio.Buffer server_key_params();

  //! Initialize for client side use.
  //!
  //! @returns
  //!   Returns @expr{1@} on success, and @expr{0@} (zero)
  //!   on failure.
  int(0..1) init_client()
  {
    return 1;
  }

  //! Initialize for server side use.
  //!
  //! @returns
  //!   Returns @expr{1@} on success, and @expr{0@} (zero)
  //!   on failure.
  int(0..1) init_server()
  {
    return 1;
  }

  //! TLS 1.3 and later.
  //!
  //! Generate a key share offer for the configured named group
  //! (currently only implemented in @[KeyShareECDHE] and @[KeyShareDHE]).
  optional void make_key_share_offer(Stdio.Buffer offer);

  //! TLS 1.3 and later.
  //!
  //! Receive a key share offer key exchange for the configured group
  //! (currently only implemented in @[KeyShareECDHE] and @[KeyShareDHE]).
  //!
  //! @note
  //!   Clears the secret state.
  //!
  //! @returns
  //!   Returns the shared pre-master key.
  optional string(8bit) receive_key_share_offer(string(8bit) offer);

  //! TLS 1.3 and later.
  //!
  //! Set the group or curve to be used.
  optional void set_group(int group);

  //! The default implementation calls @[server_key_params()] to generate
  //! the base payload.
  //!
  //! @returns
  //!   Returns the signed payload for a @[HANDSHAKE_server_key_exchange].
  string(8bit) server_key_exchange_packet(string client_random,
                                          string server_random)
  {
    Stdio.Buffer struct = server_key_params();
    if (!struct) return 0;

    session->cipher_spec->sign(session, client_random + server_random, struct);
    return struct->read();
  }

  //! @returns
  //!   Returns the premaster secret, and fills in the payload for
  //!   a @[HANDSHAKE_client_key_exchange] packet in the submitted buffer.
  //!
  //!   May return @expr{0@} (zero) to generate an @[ALERT_unexpected_message].
  string(8bit) client_key_exchange_packet(Stdio.Buffer packet_data,
					  ProtocolVersion version);

  //! @param data
  //!   Payload from a @[HANDSHAKE_client_key_exchange].
  //!
  //! @returns
  //!   Premaster secret or alert number.
  //!
  //! @note
  //!   May set @[message_was_bad] and return a fake premaster secret.
  string(8bit)|int(8bit) got_client_key_exchange(Stdio.Buffer data,
						 ProtocolVersion version);

  //! @param input
  //!   @[Stdio.Buffer] with the content of a
  //!   @[HANDSHAKE_server_key_exchange].
  //!
  //! @returns
  //!   The key exchange information should be extracted from @[input],
  //!   so that it is positioned at the signature.
  //!
  //!   Returns a new @[Stdio.Buffer] with the unsigned payload of
  //!   @[input].
  Stdio.Buffer parse_server_key_exchange(Stdio.Buffer input);

  //! @param input
  //!   @[Stdio.Buffer] with the content of a
  //!   @[HANDSHAKE_server_key_exchange].
  //!
  //! The default implementation calls @[parse_server_key_exchange()],
  //! and then verifies the signature.
  //!
  //! @returns
  //!   @int
  //!     @value 0
  //!       Returns zero on success.
  //!     @value -1
  //!       Returns negative on verification failure.
  //!   @endint
  int got_server_key_exchange(Stdio.Buffer input,
			      string client_random,
			      string server_random)
  {
    SSL3_DEBUG_MSG("SSL.Session: SERVER_KEY_EXCHANGE\n");

    Stdio.Buffer struct = parse_server_key_exchange(input);
    int verification_ok;

    if(struct)
    {
      mixed err = catch {
          verification_ok = session->cipher_spec->verify(
            session, client_random + server_random + (string)struct, input);
        };
#ifdef SSL3_DEBUG
      if( err ) {
        master()->handle_error(err);
      }
#endif
    }

    if (!verification_ok)
    {
      connection->ke = UNDEFINED;
      return -1;
    }
    return 0;
  }
}

//! Key exchange for @[KE_null].
//!
//! This is the NULL @[KeyExchange], which is only used for the
//! @[SSL_null_with_null_null] cipher suite, which is usually disabled.
class KeyExchangeNULL
{
  inherit KeyExchange;

  Stdio.Buffer server_key_params()
  {
    return Stdio.Buffer();
  }

  string(8bit) client_key_exchange_packet(Stdio.Buffer packet_data,
                                          ProtocolVersion version)
  {
    anonymous = 1;
    return "";
  }

  //! @returns
  //!   Premaster secret or alert number.
  string(8bit) got_client_key_exchange(Stdio.Buffer data,
				       ProtocolVersion version)
  {
    anonymous = 1;
    return "";
  }

  int got_server_key_exchange(Stdio.Buffer input,
			      string client_random,
			      string server_random)
  {
    return 0;
  }
}

//! Key exchange for @[KE_psk], pre shared keys.
class KeyExchangePSK
{
  inherit KeyExchange;

  protected string hint;

  Stdio.Buffer server_key_exchange_packet()
  {
    Stdio.Buffer ret = Stdio.Buffer();
    if( context->get_psk_hint )
    {
      string str = context->get_psk_hint();
      if( str )
        ret->add_hstring(str, 2);
    }
    return ret;
  }

  string(8bit) client_key_exchange_packet(Stdio.Buffer packet_data,
                                          ProtocolVersion version)
  {
    anonymous = 1;

    string id = context->get_psk_id(hint);
    if( !id ) return 0;
    packet_data->add_hstring(id, 2);

    string psk = context->get_psk(id);
    return sprintf("%2H%2H", "\0"*sizeof(psk), psk);
  }

  string(8bit)|int(8bit) got_client_key_exchange(Stdio.Buffer data,
                                                 ProtocolVersion version)
  {
    anonymous = 1;

    string psk = context->get_psk( data->read_hstring(2) );
    if( sizeof(data) )
      return ALERT_unexpected_message;
    if( !psk )
      return ALERT_unknown_psk_identity;

    return sprintf("%2H%2H", "\0"*sizeof(psk), psk);
  }

  int got_server_key_exchange(Stdio.Buffer input,
			      string client_random,
			      string server_random)
  {
    if( sizeof(input) )
      hint = input->read_hstring(2);
    if( sizeof(input) )
      return -1;

    return 0;
  }
}

//! Key exchange for @[KE_rsa].
//!
//! @[KeyExchange] that uses the Rivest Shamir Adelman algorithm.
class KeyExchangeRSA
{
  inherit KeyExchange;

  Crypto.RSA rsa; /* Key used for session key exchange. Typically the
		   * server's certified key, but may get overridden
		   * in KeyExchangeExportRSA.
		   */

  int(0..1) init_client()
  {
    rsa = session->peer_public_key;
    return 1;
  }

  int(0..1) init_server()
  {
    rsa = session->private_key;
    return 1;
  }

  Stdio.Buffer server_key_params()
  {
    SSL3_DEBUG_MSG("KE_RSA\n");
    return 0;
  }

  string(8bit) client_key_exchange_packet(Stdio.Buffer packet_data,
                                          ProtocolVersion version)
  {
    SSL3_DEBUG_MSG("client_key_exchange_packet(%O, %d.%d)\n",
		   packet_data, version>>8, version & 0xff);
    SSL3_DEBUG_MSG("KE_RSA\n");

    // NOTE: To protect against version roll-back attacks,
    //       the version sent here MUST be the same as the
    //       one in the initial handshake!
    Stdio.Buffer struct = Stdio.Buffer();
    struct->add_int(client_version, 2);
    struct->add( context->random(46) );
    string premaster_secret = struct->read();

    SSL3_DEBUG_MSG("rsa: %O\n", rsa);
    string(8bit) data = rsa->encrypt(premaster_secret);

    if(version >= PROTOCOL_TLS_1_0) {
      packet_data->add_hstring(data, 2);
    } else {
      packet_data->add(data);
    }

    return premaster_secret;
  }

  //! @returns
  //!   Premaster secret or alert number.
  string(8bit)|int(8bit) got_client_key_exchange(Stdio.Buffer input,
						 ProtocolVersion version)
  {
    string premaster_secret;

    SSL3_DEBUG_MSG("got_client_key_exchange: ke_method %d\n",
		   session->ke_method);

    SSL3_DEBUG_MSG("KE_RSA\n");

    string data;
    if(version >= PROTOCOL_TLS_1_0)
      data = input->read_hstring(2);
    else
      data = input->read();

    // Decrypt, even when we know data is incorrect, for time invariance.
    premaster_secret = rsa->decrypt(data);

    SSL3_DEBUG_MSG("premaster_secret: %O\n", premaster_secret);

    // We want both branches to execute in equal time (ignoring
    // SSL3_DEBUG in the hope it is never on in production).
    // Workaround documented in RFC 2246.
    string invalid_premaster_secret = "xx" + context->random(46);
    string tmp;
    if (premaster_secret) {
      tmp = premaster_secret;
    } else {
      tmp = invalid_premaster_secret;
    }
    string tmp2;
    if (sizeof(tmp) < 2) {
      tmp2 = invalid_premaster_secret;
    } else {
      tmp2 = tmp;
    }
    if ( `+( (sizeof(tmp2) != 48),
             (tmp2[0] != 3),
             (tmp2[1] != (client_version & 0xff)) ))
    {
      /* To avoid the chosen ciphertext attack discovered by Daniel
       * Bleichenbacher, it is essential not to send any error
       * messages back to the client until after the client's
       * Finished-message (or some other invalid message) has been
       * received.
       */
      /* Also checks for version roll-back attacks.
       */
#ifdef SSL3_DEBUG
      werror("SSL.ServerConnection: Invalid premaster_secret! "
	     "A chosen ciphertext attack?\n");
      if (premaster_secret && sizeof(premaster_secret) > 2) {
	werror("SSL.ServerConnection: Strange version (%d.%d) detected in "
	       "key exchange message (expected %d.%d).\n",
	       premaster_secret[0], premaster_secret[1],
	       client_version>>8, client_version & 0xff);
      }
#endif

      premaster_secret = invalid_premaster_secret;
      message_was_bad = 1;
    } else {
      tmp = invalid_premaster_secret;
      message_was_bad = 0;
    }
    connection->ke = this;

    return premaster_secret;
  }
}

//! Key exchange for @[KE_rsa_export].
//!
//! @[KeyExchange] that uses the Rivest Shamir Adelman algorithm,
//! but limited to 512 bits for encryption and decryption.
class KeyExchangeExportRSA
{
  inherit KeyExchangeRSA;

  Stdio.Buffer server_key_params()
  {
    SSL3_DEBUG_MSG("KE_EXPORT_RSA\n");

    rsa = context->get_export_rsa_key();
    SSL3_DEBUG_MSG("Sending a server key exchange-message, "
		   "with a %d-bits key.\n",
		   rsa->key_size());
    Stdio.Buffer output = Stdio.Buffer();
    output->add_hint(rsa->get_n(),2);
    output->add_hint(rsa->get_e(),2);
    return output;
  }

  Stdio.Buffer parse_server_key_exchange(Stdio.Buffer input)
  {
    SSL3_DEBUG_MSG("KE_RSA\n");

    string n = input->read_hstring(2);
    string e = input->read_hstring(2);

    rsa = Crypto.RSA()->set_public_key(Gmp.mpz(n,256), Gmp.mpz(e,256));

    Stdio.Buffer output = Stdio.Buffer();
    output->add_hstring(n, 2);
    output->add_hstring(e, 2);
    return output;
  }
}

//! Key exchange for @[KE_rsa_psk].
class KeyExchangeRSAPSK
{
  inherit KeyExchangePSK;

  string(8bit) client_key_exchange_packet(Stdio.Buffer packet_data,
                                          ProtocolVersion version)
  {
    string id = context->get_psk_id(hint);
    if( !id ) return 0;
    packet_data->add_hstring(id, 2);

    // PreMasterSecret/EncryptedPreMasterSecret from TLS 1.0.
    Stdio.Buffer struct = Stdio.Buffer();
    struct->add_int(client_version, 2);
    struct->add( context->random(46) );
    string premaster_secret = struct->read();
    packet_data->add( session->peer_public_key->encrypt(premaster_secret) );

    string psk = context->get_psk(id);
    Stdio.Buffer master_secret = Stdio.Buffer();
    master_secret->add_hstring(premaster_secret, 2);
    master_secret->add_hstring(psk, 2);
    return master_secret->read();
  }

  string(8bit)|int(8bit) got_client_key_exchange(Stdio.Buffer data,
                                                 ProtocolVersion version)
  {
    string psk = context->get_psk( data->read_hstring(2) );
    if( !psk )
      return ALERT_unknown_psk_identity;

    string premaster_secret = session->private_key->decrypt( data->read() ) ||
      "xx";

    if ( `+( (sizeof(premaster_secret) != 48),
             (premaster_secret[0] != 3),
             (premaster_secret[1] != (client_version & 0xff)) ))
    {
      premaster_secret = context->random(48);
      message_was_bad = 1;
      connection->ke = UNDEFINED;
    } else {
      string timing_attack_mitigation = context->random(48);
      message_was_bad = 0;
      connection->ke = this;
    }

    Stdio.Buffer master_secret = Stdio.Buffer();
    master_secret->add_hstring(premaster_secret, 2);
    master_secret->add_hstring(psk, 2);
    return master_secret->read();
  }
}

//! KeyExchange for @[KE_dhe_rsa], @[KE_dhe_dss] and @[KE_dh_anon].
//!
//! KeyExchange that uses Diffie-Hellman to generate an Ephemeral key.
class KeyExchangeDHE
{
  inherit KeyExchange;

  //! Finite field Diffie-Hellman parameters.
  Crypto.DH.Parameters parameters;

  Gmp.mpz our; /* Our value */
  private Gmp.smpz other; /* Other party's value */
  Gmp.mpz secret; /* our =  g ^ secret mod p */

  protected void new_secret()
  {
    [our, secret] = parameters->generate_keypair(context->random);
  }

  protected Gmp.mpz get_shared()
  {
    Gmp.mpz shared = other->powm(secret, parameters->p);
    secret = 0;
    return shared;
  }

  //! Set the value received from the peer.
  //!
  //! @returns
  //!   Returns @expr{1@} if @[o] is valid for the set @[parameters].
  //!
  //!   Otherwise returns @[UNDEFINED].
  protected int(0..1) set_other(Gmp.smpz o)
  {
    if ((o <= 1) || (o >= (parameters->p - 1))) {
      // Negotiated FF DHE Parameters Draft 4 3:
      //   If the selected group matches an offered FFDHE group exactly, the
      //   the client MUST verify that dh_Ys is in the range 1 < dh_Ys < dh_p
      //   - 1.  If dh_Ys is not in this range, the client MUST terminate the
      //   connection with a fatal handshake_failure(40) alert.
      //
      // Negotiated FF DHE Parameters Draft 4 4:
      //   When a compatible server selects an FFDHE group from among a
      //   client's Supported Groups, and the client sends a ClientKeyExchange,
      //   the server MUST verify that 1 < dh_Yc < dh_p - 1. If it is out of
      //   range, the server MUST terminate the connection with fatal
      //   handshake_failure(40) alert.
      return UNDEFINED;
    }
    other = o;
    return 1;
  }

  Stdio.Buffer server_key_params()
  {
    SSL3_DEBUG_MSG("KE_DHE\n");
    // anonymous, not used on the server, but here for completeness.
    anonymous = 1;

    // NIST SP800-57 5.6.1
    // { symmetric key length, p limit, q limit }
    constant nist_strength = ({
      ({ 80,   1024, 160 }),
      ({ 112,  2048, 224 }),
      ({ 128,  3072, 256 }),
      ({ 192,  7680, 384 }),
      ({ 256, 15360, 511 }),
    });
    int key_strength = CIPHER_effective_keylengths
      [ CIPHER_SUITES[ session->cipher_suite ][1] ];
    int target_p, target_q;
    foreach(nist_strength, [int key, target_p, target_q])
      if( key_strength <= key ) break;

    Crypto.DH.Parameters p;
    foreach( context->ffdhe_groups, int g )
    {
      Crypto.DH.Parameters o =
	FFDHE_GROUPS[g] || context->private_ffdhe_groups[g];
      if (!o) continue;	// Paranoia.
      if( !p || o->p->size()>p->p->size() ||
          (o->p->size()==p->p->size() && o->q->size()>p->q->size()) )
        p = o;
      if( p->p->size() >= target_p && p->q->size() >= target_q )
        break;
    }

    // FIXME: Fall back to just selecting a group and pretend
    //        not to support the FFDHE extension?
    if(!p) error("No suitable DH group in Context.\n");
    parameters = p;
    new_secret();

    Stdio.Buffer output = Stdio.Buffer();
    output->add_hint(parameters->p, 2);
    output->add_hint(parameters->g, 2);
    output->add_hint(our, 2);
    return output;
  }

  string(8bit) client_key_exchange_packet(Stdio.Buffer packet_data,
                                          ProtocolVersion version)
  {
    SSL3_DEBUG_MSG("client_key_exchange_packet(%O, %d.%d)\n",
		   packet_data, version>>8, version & 0xff);

    SSL3_DEBUG_MSG("KE_DH/KE_DHE\n");
    anonymous = 1;
    if (!parameters) {
      SSL3_DEBUG_MSG("Invalid DH exchange.\n");
      return 0;
    }
    new_secret();

    string premaster_secret = get_shared()->digits(256);

    packet_data->add_hint(our, 2);
    return premaster_secret;
  }

  //! @returns
  //!   Premaster secret or alert number.
  string(8bit)|int(8bit) got_client_key_exchange(Stdio.Buffer input,
						 ProtocolVersion version)
  {
    string premaster_secret;

    SSL3_DEBUG_MSG("got_client_key_exchange: ke_method %d\n",
		   session->ke_method);

    SSL3_DEBUG_MSG("KE_DH/KE_DHE\n");
    anonymous = 1;

    if (!sizeof(input))
    {
      /* Implicit encoding; Should never happen unless we have
       * requested and received a client certificate of type
       * rsa_fixed_dh or dss_fixed_dh. Not supported. */
      SSL3_DEBUG_MSG("SSL.ServerConnection: Client uses implicit encoding if its DH-value.\n"
		     "               Hanging up.\n");
      connection->ke = UNDEFINED;
      return ALERT_certificate_unknown;
    }

    if (!set_other(Gmp.smpz(input->read_hint(2)))) {
      connection->ke = UNDEFINED;
      return ALERT_handshake_failure;
    }

    if(sizeof(input))
    {
      connection->ke = UNDEFINED;
      return ALERT_handshake_failure;
    }

    premaster_secret = get_shared()->digits(256);
    parameters = 0;

    return premaster_secret;
  }

  Stdio.Buffer parse_server_key_exchange(Stdio.Buffer input)
  {
    SSL3_DEBUG_MSG("KE_DHE\n");
    string p = input->read_hstring(2);
    string g = input->read_hstring(2);
    string o = input->read_hstring(2);

    Crypto.DH.Parameters params = Crypto.DH.Parameters(Gmp.mpz(p,256),
                                                       Gmp.mpz(g,256));
    if( !validate_dh(params, session, context) )
    {
      SSL3_DEBUG_MSG("DH parameters not correct or not secure.\n");
      return 0;
    }

    parameters = params;
    if (!set_other(Gmp.smpz(o,256))) {
      SSL3_DEBUG_MSG("DH Ys not valid for set parameters.\n");
      parameters = 0;
      return 0;
    }

    Stdio.Buffer output = Stdio.Buffer();
    output->add_hstring(p, 2);
    output->add_hstring(g, 2);
    output->add_hstring(o, 2);
    return output;
  }
}

//! Key exchange for @[KE_dhe_psk].
class KeyExchangeDHEPSK
{
  inherit KeyExchangePSK : PSK;
  inherit KeyExchangeDHE : DHE;

  protected void create(object context, object session, object connection,
                        ProtocolVersion client_version)
  {
    PSK::create(context, session, connection, client_version);
    DHE::create(context, session, connection, client_version);
  }

  Stdio.Buffer server_key_exchange_packet()
  {
    Stdio.Buffer psk = PSK::server_key_exchange_packet();
    if( !sizeof(psk) )
      psk->add_int(0, 2);
    Stdio.Buffer dhe = DHE::server_key_params();
    return psk->add(dhe);
  }

  string(8bit) client_key_exchange_packet(Stdio.Buffer packet_data,
                                          ProtocolVersion version)
  {
    string(8bit) id = context->get_psk_id(hint);
    if( !id ) return 0;
    packet_data->add_hstring(id, 2);

    string(8bit) s = DHE::client_key_exchange_packet(packet_data, version);
    if( !s ) return 0;

    string(8bit) psk = context->get_psk(id);
    return s + sprintf("%2H", psk);
  }

  string(8bit)|int(8bit) got_client_key_exchange(Stdio.Buffer data,
                                                 ProtocolVersion version)
  {
    string(8bit) psk = context->get_psk( data->read_hstring(2) );
    if( !psk )
      return ALERT_unknown_psk_identity;

    string(8bit)|int(8bit) s = DHE::got_client_key_exchange(data, version);
    if( intp(s) ) return s;

    return [string(8bit)]s + sprintf("%2H", psk);
  }

  int got_server_key_exchange(Stdio.Buffer input,
			      string client_random,
			      string server_random)
  {
    hint = input->read_hstring(2);
    DHE::parse_server_key_exchange(input);
    return 0;
  }
}

//! Key exchange for @[KE_dh_dss] and @[KE_dh_dss].
//!
//! @[KeyExchange] that uses Diffie-Hellman with a key from
//! a DSS certificate.
class KeyExchangeDH
{
  inherit KeyExchangeDHE;

  int(0..1) init_server()
  {
    parameters = Crypto.DH.Parameters(session->private_key);
    secret = session->private_key->get_x();
    return ::init_server();
  }

  int(0..1) init_client()
  {
    Crypto.DH.Parameters p = Crypto.DH.Parameters(session->peer_public_key);
    if( !validate_dh(p, session, context) )
    {
      SSL3_DEBUG_MSG("DH parameters not correct or not secure.\n");
      return 0;
    }
    parameters = p;
    if (!set_other(Gmp.smpz(session->peer_public_key->get_y()))) {
      SSL3_DEBUG_MSG("DH Ys not valid for set parameters.\n");
      parameters = 0;
      return 0;
    }
    return ::init_client();
  }

  Stdio.Buffer server_key_params()
  {
    SSL3_DEBUG_MSG("KE_DH\n");
    // RFC 4346 7.4.3:
    // It is not legal to send the server key exchange message for the
    // following key exchange methods:
    //
    //   RSA
    //   DH_DSS
    //   DH_RSA
    return 0;
  }

  Stdio.Buffer parse_server_key_exchange(Stdio.Buffer input)
  {
    SSL3_DEBUG_MSG("KE_DH\n");
    // RFC 4346 7.4.3:
    // It is not legal to send the server key exchange message for the
    // following key exchange methods:
    //
    //   RSA
    //   DH_DSS
    //   DH_RSA
    error("Invalid message.\n");
  }
}

class KeyShareDHE
{
  inherit KeyExchangeDHE;

  int group;

  void make_key_share_offer(Stdio.Buffer offer)
  {
    offer->add_int(group, 2);
    offer->add_hint(our, 2);
  }

  string(8bit) receive_key_share_offer(string(8bit) offer)
  {
    set_other(Gmp.smpz(offer, 256));

    string(8bit) premaster_secret = get_shared()->digits(256);

    parameters = UNDEFINED;

    return premaster_secret;
  }

  void set_group(int g)
  {
    group = g;
    parameters = FFDHE_GROUPS[g] || context->private_ffdhe_groups[g];
    if (!parameters) error("Unsupported FF-DHE group.\n");
    new_secret();
  }
}

#if constant(Crypto.ECC.Curve)
//! KeyExchange for @[KE_ecdhe_rsa] and @[KE_ecdhe_ecdsa].
//!
//! KeyExchange that uses Elliptic Curve Diffie-Hellman to
//! generate an Ephemeral key.
class KeyExchangeECDHE
{
  inherit KeyExchange;

  protected Gmp.mpz|string(8bit) secret;
  protected Crypto.ECC.Curve.Point point;

  int(0..1) init_client()
  {
    session->curve =
      ([object(Crypto.ECC.Curve.ECDSA)]session->peer_public_key)->
      get_curve();
    return ::init_client();
  }

  Stdio.Buffer server_key_params()
  {
    SSL3_DEBUG_MSG("KE_ECDHE\n");
    // anonymous, not used on the server, but here for completeness.
    anonymous = 1;

    // Select a suitable curve.
    int c;
    switch(session->cipher_spec->key_bits) {
    case 257..:
      c = GROUP_secp521r1;
      break;
    case 129..256:
      // Suite B requires SECP384r1
      c = GROUP_secp384r1;
      break;
    case ..128:
      c = GROUP_secp256r1;
      break;
    }

    if (!has_value(session->ecc_curves, c)) {
      // Preferred curve not available -- Select the strongest available.
      c = session->ecc_curves[0];
    }
    session->curve = ECC_CURVES[c];

    SSL3_DEBUG_MSG("Curve: %s: %O\n", fmt_constant(c, "GROUP"), session->curve);

    secret = session->curve->new_scalar(context->random);
    Crypto.ECC.Curve.Point p = session->curve * secret;

    SSL3_DEBUG_MSG("secret: %O\n", secret);
    SSL3_DEBUG_MSG("x: %O\n", p->get_x());
    SSL3_DEBUG_MSG("y: %O\n", p->get_y());
    SSL3_DEBUG_MSG("p: %O\n", p);

    Stdio.Buffer struct = Stdio.Buffer();
    struct->add_int(CURVETYPE_named_curve, 1);
    struct->add_int(c, 2);
    struct->add_hstring(p->encode(), 1);
    return struct;
  }

  string(8bit) client_key_exchange_packet(Stdio.Buffer packet_data,
                                          ProtocolVersion version)
  {
    SSL3_DEBUG_MSG("client_key_exchange_packet(%O, %d.%d)\n",
		   packet_data, version>>8, version & 0xff);
    SSL3_DEBUG_MSG("KE_ECDHE\n");

    object(Gmp.mpz)|string(8bit) secret =
      session->curve->new_scalar(context->random);
    Crypto.ECC.Curve.Point p = session->curve * secret;

    SSL3_DEBUG_MSG("Client point: %O\n"
		   "encoded, hex: %s (%d bytes)\n",
		   p, String.string2hex(p->encode()), sizeof(p->encode()));

    // RFC 4492 5.10:
    // Note that this octet string (Z in IEEE 1363 terminology) as
    // output by FE2OSP, the Field Element to Octet String
    // Conversion Primitive, has constant length for any given
    // field; leading zeros found in this octet string MUST NOT be
    // truncated.
    string premaster_secret = (point*secret)->get_x_str();
    secret = 0;

    packet_data->add_hstring(p->encode(), 1);
    return premaster_secret;
  }

  //! @returns
  //!   Premaster secret or alert number.
  string(8bit)|int(8bit) got_client_key_exchange(Stdio.Buffer data,
						 ProtocolVersion version)
  {
    SSL3_DEBUG_MSG("got_client_key_exchange: ke_method %d\n",
		   session->ke_method);

    SSL3_DEBUG_MSG("KE_ECDHE\n");

    if (!sizeof(data))
    {
      /* Implicit encoding; Should never happen unless we have
       * requested and received a client certificate of type
       * rsa_fixed_dh or dss_fixed_dh. Not supported. */
      SSL3_DEBUG_MSG("SSL.ServerConnection: Client uses implicit encoding if its DH-value.\n"
		     "               Hanging up.\n");
      connection->ke = UNDEFINED;
      return ALERT_certificate_unknown;
    }

    string premaster_secret;

    Crypto.ECC.Curve.Point point;
    catch {
      point = session->curve->Point(data->read_hbuffer(1));
    };
    if (!point)
      return ALERT_decode_error;

    // RFC 4492 5.10:
    // Note that this octet string (Z in IEEE 1363 terminology) as
    // output by FE2OSP, the Field Element to Octet String
    // Conversion Primitive, has constant length for any given
    // field; leading zeros found in this octet string MUST NOT be
    // truncated.
    premaster_secret = (point*secret)->get_x_str();

    return premaster_secret;
  }

  Stdio.Buffer parse_server_key_exchange(Stdio.Buffer input)
  {
    SSL3_DEBUG_MSG("KE_ECDHE\n");

    SSL3_DEBUG_MSG("input: %O\n"
		   "data: %s\n",
		   input, String.string2hex(sprintf("%s", input)));

    Stdio.Buffer.RewindKey key = input->rewind_key();
    int len = sizeof(input);

    // First the curve.
    switch(input->read_int(1)) {
    case CURVETYPE_named_curve:
      int c = input->read_int(2);
      if (has_value(context->ecc_curves, c)) {
	// Only look up curves that we are configured to support.
	session->curve = ECC_CURVES[c];
      }
      if (!session->curve) {
	connection->ke = UNDEFINED;
	error("Unsupported curve: %s.\n", fmt_constant(c, "GROUP"));
      }
      SSL3_DEBUG_MSG("Curve: %O (%O: %s)\n",
		     session->curve, c, fmt_constant(c, "GROUP"));
      break;
    default:
      connection->ke = UNDEFINED;
      error("Invalid curve encoding.\n");
      break;
    }

    // Then the point.
    catch {
      Stdio.Buffer sub = input->read_hbuffer(1);
      SSL3_DEBUG_MSG("Raw point: %O\n", sub);
      point = session->curve->Point(sub);
      SSL3_DEBUG_MSG("Point: %O\n", point);
    };
    if (!point)
      return 0;

    SSL3_DEBUG_MSG("input: %O\n", input);
    len = len - sizeof(input);
    key->rewind();
    SSL3_DEBUG_MSG("len: %d\n", len);
    SSL3_DEBUG_MSG("input (rewound): %O\n"
		   "data: %s\n",
		   input, String.string2hex(sprintf("%s", input)[..len-1]));
    Stdio.Buffer res = input->read_buffer(len);
    SSL3_DEBUG_MSG("Result: %O %s (%d bytes)\n",
		   res, String.string2hex(sprintf("%s", res)), len);
    return res;
  }
}

//! Key exchange for @[KE_ecdhe_psk].
class KeyExchangeECDHEPSK
{
  inherit KeyExchangePSK : PSK;
  inherit KeyExchangeECDHE : ECDHE;

  protected void create(object context, object session, object connection,
                        ProtocolVersion client_version)
  {
    PSK::create(context, session, connection, client_version);
    ECDHE::create(context, session, connection, client_version);
  }

  Stdio.Buffer server_key_exchange_packet()
  {
    Stdio.Buffer psk = PSK::server_key_exchange_packet();
    if( !sizeof(psk) )
      psk->add_int(0, 2);
    Stdio.Buffer ecdhe = ECDHE::server_key_params();
    return psk->add(ecdhe);
  }

  string(8bit) client_key_exchange_packet(Stdio.Buffer packet_data,
                                          ProtocolVersion version)
  {
    string(8bit) id = context->get_psk_id(hint);
    if( !id ) return 0;
    packet_data->add_hstring(id, 2);

    string(8bit) z = ECDHE::client_key_exchange_packet(packet_data, version);
    if( !z ) return 0;

    string(8bit) psk = context->get_psk(id);
    return sprintf("%2H%2H", z, psk);
  }

  string(8bit)|int(8bit) got_client_key_exchange(Stdio.Buffer data,
                                                 ProtocolVersion version)
  {
    string(8bit) psk = context->get_psk( data->read_hstring(2) );
    if( !psk )
      return ALERT_unknown_psk_identity;

    string(8bit)|int(8bit) z = ECDHE::got_client_key_exchange(data, version);
    if( intp(z) ) return z;

    return sprintf("%2H%2H", [string(8bit)]z, psk);
  }

  int got_server_key_exchange(Stdio.Buffer input,
			      string client_random,
			      string server_random)
  {
    hint = input->read_hstring(2);
    ECDHE::parse_server_key_exchange(input);
    return 0;
  }
}

//! KeyExchange for @[KE_ecdh_rsa] and @[KE_ecdh_ecdsa].
//!
//! NB: The only difference between the two is whether the certificate
//!     is signed with RSA or ECDSA.
//!
//! This KeyExchange uses the Elliptic Curve parameters from
//! the ECDSA certificate on the server side, and ephemeral
//! parameters on the client side.
class KeyExchangeECDH
{
  inherit KeyExchangeECDHE;

  int(0..1) init_server()
  {
    secret = session->private_key->get_private_key();
    return ::init_server();
  }

  int(0..1) init_client()
  {
    point = session->peer_public_key->get_point();
    return ::init_client();
  }

  Stdio.Buffer server_key_params()
  {
    SSL3_DEBUG_MSG("KE_ECDH\n");
    // RFC 4492 2.1:
    // A ServerKeyExchange MUST NOT be sent (the server's certificate
    // contains all the necessary keying information required by the client
    // to arrive at the premaster secret).
    return 0;
  }

  Stdio.Buffer parse_server_key_exchange(Stdio.Buffer input)
  {
    SSL3_DEBUG_MSG("KE_ECDH\n");
    // RFC 4492 2.1:
    // A ServerKeyExchange MUST NOT be sent (the server's certificate
    // contains all the necessary keying information required by the client
    // to arrive at the premaster secret).
    error("Invalid message.\n");
  }
}

class KeyShareECDHE
{
  inherit KeyExchangeECDHE;

  int group;
  Crypto.ECC.Curve curve;

  void make_key_share_offer(Stdio.Buffer offer)
  {
    Crypto.ECC.Curve.Point p = curve * secret;

    SSL3_DEBUG_MSG("secret: %O\n", secret);
    SSL3_DEBUG_MSG("x: %O\n", p->get_x());
    SSL3_DEBUG_MSG("y: %O\n", p->get_y());

    offer->add_int(group, 2);
    offer->add_hstring(p->encode(), 2);
  }

  string(8bit) receive_key_share_offer(string(8bit) offer)
  {
    catch {
      point = curve->Point(Stdio.Buffer(offer));
    };
    if (!point)
      return 0;

    // RFC 4492 5.10:
    // Note that this octet string (Z in IEEE 1363 terminology) as
    // output by FE2OSP, the Field Element to Octet String
    // Conversion Primitive, has constant length for any given
    // field; leading zeros found in this octet string MUST NOT be
    // truncated.
    string premaster_secret =
      sprintf("%*c", (curve->size() + 7)>>3, (point*secret)->get_x());
    secret = 0;

    return premaster_secret;
  }

  void set_group(int c)
  {
    group = c;
    curve = ECC_CURVES[c];
    if (!curve) error("Unsupported curve.\n");
    secret = curve->new_scalar(context->random);
  }
}

#endif /* Crypto.ECC.Curve */

#if constant(GSSAPI)

//! Key exchange for @[KE_krb].
//!
//! @[KeyExchange] that uses Kerberos (@rfc{2712@}).
class KeyExchangeKRB
{
  inherit KeyExchange;

  GSSAPI.Context gss_context;

  Stdio.Buffer server_key_params()
  {
    SSL3_DEBUG_MSG("KE_KRB\n");

    // RFC 2712 3:
    //
    // The server's certificate, the client CertificateRequest, and
    // the ServerKeyExchange shown in Figure 1 will be omitted since
    // authentication and the establishment of a master secret will be
    // done using the client's Kerberos credentials for the TLS server.

    return 0;
  }

  string(8bit) client_key_exchange_packet(Stdio.Buffer packet_data,
                                          ProtocolVersion version)
  {
    SSL3_DEBUG_MSG("client_key_exchange_packet(%O, %d.%d)\n",
		   packet_data, version>>8, version & 0xff);

    SSL3_DEBUG_MSG("KE_KRB\n");

    // FIXME: The second argument to InitContext is required,
    //        and should be host/MachineName@Realm.
    gss_context = GSSAPI.InitContext();

    string(8bit) token = gss_context->init();

    Stdio.Buffer struct = Stdio.Buffer();

    // NOTE: To protect against version roll-back attacks,
    //       the version sent here MUST be the same as the
    //       one in the initial handshake!
    struct->add_int(client_version, 2);
    struct->add(context->random(46));
    string(8bit) premaster_secret = struct->read();

    string(8bit) encrypted_premaster_secret =
      gss_context->wrap(premaster_secret, 1);

    packet_data->add_hstring(token, 2);
    packet_data->add_hstring("", 2);	// Authenticator.
    packet_data->add_hstring(encrypted_premaster_secret, 2);
    return premaster_secret;
  }

  //! @returns
  //!   Premaster secret or alert number.
  string(0..255)|int got_client_key_exchange(Stdio.Buffer input,
					     ProtocolVersion version)
  {
    SSL3_DEBUG_MSG("got_client_key_exchange: ke_method %d\n",
		   session->ke_method);

    SSL3_DEBUG_MSG("KE_KRB\n");

    string ticket = input->read_hstring(2);
    string authenticator = input->read_hstring(2);
    string encrypted_premaster_secret = input->read_hstring(2);

    gss_context = GSSAPI.AcceptContext();
    gss_context->accept(ticket);

    /* Decrypt the premaster_secret */
    SSL3_DEBUG_MSG("encrypted premaster_secret: %O\n",
		   encrypted_premaster_secret);

    string(8bit) premaster_secret =
      gss_context->unwrap(encrypted_premaster_secret, 1);

    SSL3_DEBUG_MSG("premaster_secret: %O\n", premaster_secret);

    // We want both branches to execute in equal time (ignoring
    // SSL3_DEBUG in the hope it is never on in production).
    // Workaround documented in RFC 2246.
    if ( `+( !premaster_secret,
             (sizeof(premaster_secret) != 48),
             (premaster_secret[0] != 3),
             (premaster_secret[1] != (client_version & 0xff)) ))
    {
      /* To avoid the chosen ciphertext attack discovered by Daniel
       * Bleichenbacher, it is essential not to send any error
       * messages back to the client until after the client's
       * Finished-message (or some other invalid message) has been
       * received.
       */
      /* Also checks for version roll-back attacks.
       */
#ifdef SSL3_DEBUG
      werror("SSL.handshake: Invalid premaster_secret! "
	     "A chosen ciphertext attack?\n");
      if (premaster_secret && sizeof(premaster_secret) > 2) {
	werror("SSL.handshake: Strange version (%d.%d) detected in "
	       "key exchange message (expected %d.%d).\n",
	       premaster_secret[0], premaster_secret[1],
	       client_version>>8, client_version & 0xff);
      }
#endif

      premaster_secret = context->random(48);
      message_was_bad = 1;
      connection->ke = UNDEFINED;

    } else {
      string timing_attack_mitigation = context->random(48);
      message_was_bad = 0;
      connection->ke = this;
    }

    return premaster_secret;
  }

  Stdio.Buffer parse_server_key_exchange(Stdio.Buffer input)
  {
    SSL3_DEBUG_MSG("KE_KRB\n");
    error("Invalid message.\n");
  }
}
#endif /* GSSAPI */

//! MAC using SHA.
//!
//! @note
//!   Note: This uses the algorithm from the SSL 3.0 draft.
class MACsha
{
  inherit MACAlgorithm;

  protected constant pad_1 = "6" * 40;
  protected constant pad_2 = "\\" * 40;

  protected Crypto.Hash algorithm = Crypto.SHA1;
  protected string secret;

  constant hash_header_size = 11;

  string(8bit) hash_raw(string(8bit) data)
  {
    return algorithm->hash(data);
  }

  string(8bit) hash_packet(object packet, int|void adjust_len)
  {
    if (undefinedp(packet->seq_num)) {
      error("Packet %O has uninitialized sequence number.\n", packet);
    }
    string s = sprintf("%8c%c%2c",
		       packet->seq_num,
		       packet->content_type,
		       sizeof(packet->fragment) + adjust_len);
    Crypto.Hash.State h = algorithm();
    h->update(secret);
    h->update(pad_1);
    h->update(s);
    h->update(packet->fragment);
    return hash_raw(secret + pad_2 + h->digest());
  }

  string(8bit) hash(string(8bit) data)
  {
    return hash_raw(secret + pad_2 +
		hash_raw(data + secret + pad_1));
  }

  int(1..) block_size()
  {
    return algorithm->block_size();
  }

  protected void create (string|void s)
  {
    secret = s || "";
  }
}

//! MAC using MD5.
//!
//! @note
//!   Note: This uses the algorithm from the SSL 3.0 draft.
class MACmd5 {
  inherit MACsha;

  protected constant pad_1 = "6" * 48;
  protected constant pad_2 = "\\" * 48;

  protected Crypto.Hash algorithm = Crypto.MD5;
}

//! HMAC using SHA.
//!
//! This is the MAC algorithm used by TLS 1.0 and later.
class MAChmac_sha {
  inherit MACAlgorithm;

  protected Crypto.Hash algorithm = Crypto.SHA1;
  protected Crypto.MAC.State hmac;

  constant hash_header_size = 13;

  string hash_raw(string data)
  {
    return algorithm->hash(data);
  }

  string hash(string data)
  {
    return hmac(data);
  }

  string hash_packet(object packet, int|void adjust_len)
  {
    if (undefinedp(packet->seq_num)) {
      error("Packet %O has uninitialized sequence number.\n", packet);
    }
    SSL3_DEBUG_CRYPT_MSG("HMAC header: %x\n",
                         sprintf("%8c%c%2c%2c",
				 packet->seq_num,
                                 packet->content_type,
                                 packet->protocol_version,
                                 sizeof(packet->fragment) + adjust_len));
    hmac->update( sprintf("%8c%c%2c%2c",
			  packet->seq_num,
                          packet->content_type,
                          packet->protocol_version,
                          sizeof(packet->fragment) + adjust_len));
    SSL3_DEBUG_CRYPT_MSG("HMAC data: %x\n", packet->fragment);
    hmac->update( packet->fragment );
    return hmac->digest();
  }

  int(1..) block_size()
  {
    return algorithm->block_size();
  }

  //!
  protected void create(string|void s) {
    hmac = algorithm->HMAC( s||"" );
  }
}

//! HMAC using MD5.
//!
//! This is the MAC algorithm used by TLS 1.0 and later.
class MAChmac_md5 {
  inherit MAChmac_sha;
  protected Crypto.Hash algorithm = Crypto.MD5;
}

//! HMAC using SHA256.
//!
//! This is the MAC algorithm used by some cipher suites in TLS 1.2 and later.
class MAChmac_sha256 {
  inherit MAChmac_sha;
  protected Crypto.Hash algorithm = Crypto.SHA256;
}

#if constant(Crypto.SHA384)
//! HMAC using SHA384.
//!
//! This is a MAC algorithm used by some cipher suites in TLS 1.2 and later.
class MAChmac_sha384 {
  inherit MAChmac_sha;
  protected Crypto.Hash algorithm = Crypto.SHA384;
}
#endif

#if constant(Crypto.SHA512)
//! HMAC using SHA512.
//!
//! This is a MAC algorithm used by some cipher suites in TLS 1.2 and later.
class MAChmac_sha512 {
  inherit MAChmac_sha;
  protected Crypto.Hash algorithm = Crypto.SHA512;
}
#endif

//! Hashfn is either a @[Crypto.MD5], @[Crypto.SHA] or @[Crypto.SHA256].
protected string(8bit) P_hash(Crypto.Hash hashfn,
                              string(8bit) secret,
                              string(8bit) seed, int len)
{
  Crypto.MAC.State hmac=hashfn->HMAC(secret);
  string temp=seed;
  string res="";

  while( sizeof(res)<len )
  {
    temp=hmac(temp);
    res+=hmac(temp+seed);
  }
  return res[..(len-1)];
}

//! This Pseudo Random Function is used to derive secret keys in SSL 3.0.
//!
//! @note
//!   The argument @[label] is ignored.
string(8bit) prf_ssl_3_0(string(8bit) secret,
                         string(8bit) label,
                         string(8bit) seed,
                         int len)
{
  string res = "";
  for (int i = 1; sizeof(res) < len; i++) {
    string cookie = (string)allocate(i, i + 64);
    res += Crypto.MD5.hash(secret +
			   Crypto.SHA1.hash(cookie + secret + seed));
  }
  return res[..len-1];
}

//! This Pseudo Random Function is used to derive secret keys
//! in TLS 1.0 and 1.1.
string(8bit) prf_tls_1_0(string(8bit) secret,
                         string(8bit) label,
                         string(8bit) seed,
                         int len)
{
  string s1=secret[..(int)(ceil(sizeof(secret)/2.0)-1)];
  string s2=secret[(int)(floor(sizeof(secret)/2.0))..];

  string a=P_hash(Crypto.MD5, s1, label+seed, len);
  string b=P_hash(Crypto.SHA1, s2, label+seed, len);

  return a ^ b;
}

//! This Pseudo Random Function is used to derive secret keys in TLS 1.2.
string(8bit) prf_tls_1_2(string(8bit) secret,
                         string(8bit) label,
                         string(8bit) seed,
                         int len)
{
  return P_hash(Crypto.SHA256, secret, label + seed, len);
}

#if constant(Crypto.SHA384)
//! This Pseudo Random Function is used to derive secret keys
//! for some ciphers suites defined after TLS 1.2.
string(8bit) prf_sha384(string(8bit) secret,
                        string(8bit) label,
                        string(8bit) seed,
                        int len)
{
  return P_hash(Crypto.SHA384, secret, label + seed, len);
}
#endif

#if constant(Crypto.SHA512)
//! This Pseudo Random Function could be used to derive secret keys
//! for some ciphers suites defined after TLS 1.2.
string(8bit) prf_sha512(string(8bit) secret,
                        string(8bit) label,
                        string(8bit) seed,
                        int len)
{
  return P_hash(Crypto.SHA512, secret, label + seed, len);
}
#endif

//!
class DES
{
  inherit Crypto.DES.CBC.Buffer.State;

  this_program set_encrypt_key(string k)
  {
    ::set_encrypt_key(Crypto.DES.fix_parity(k));
    return this;
  }

  this_program set_decrypt_key(string k)
  {
    ::set_decrypt_key(Crypto.DES.fix_parity(k));
    return this;
  }
}

//!
class DES3
{
  inherit Crypto.DES3.CBC.Buffer.State;

  this_program set_encrypt_key(string k)
  {
    ::set_encrypt_key(Crypto.DES3.fix_parity(k));
    return this;
  }

  this_program set_decrypt_key(string k)
  {
    ::set_decrypt_key(Crypto.DES3.fix_parity(k));
    return this;
  }
}

#if constant(Crypto.Arctwo)
//!
class RC2
{
  inherit Crypto.Arctwo.CBC.Buffer.State;

  this_program set_encrypt_key(string k)
  {
    ::set_encrypt_key(k, 128);
    return this;
  }

  this_program set_decrypt_key(string k)
  {
    ::set_decrypt_key(k, 128);
    return this;
  }
}
#endif /* Crypto.Arctwo */

//! Lookup the crypto parameters for a cipher suite.
//!
//! @param suite
//!   Cipher suite to lookup.
//!
//! @param version
//!   Version of the SSL/TLS protocol to support.
//!
//! @param signature_algorithms
//!   The set of @[SignatureScheme] values that
//!   are supported by the other end.
//!
//! @param max_hash_size
//!   The maximum hash size supported for the signature algorithm.
//!
//! @returns
//!   Returns @expr{0@} (zero) for unsupported combinations, otherwise
//!   returns an initialized @[CipherSpec] for the @[suite].
CipherSpec lookup(int suite, ProtocolVersion|int version,
                  array(int) signature_algorithms,
                  int max_hash_size)
{
  CipherSpec res = CipherSpec();

  array algorithms = CIPHER_SUITES[suite];
  if (!algorithms)
    return 0;

  int ke_method = algorithms[0];

  switch(algorithms[1])
  {
#if constant(Crypto.Arctwo)
  case CIPHER_rc2_40:
    res->bulk_cipher_algorithm = RC2;
    res->cipher_type = CIPHER_block;
    res->is_exportable = 1;
    res->key_material = 16;
    res->iv_size = 8;
    res->key_bits = 40;
    break;
#endif
  case CIPHER_rc4_40:
    res->bulk_cipher_algorithm = Crypto.Arcfour.State;
    res->cipher_type = CIPHER_stream;
    res->is_exportable = 1;
    res->key_material = 16;
    res->iv_size = 0;
    res->key_bits = 40;
    break;
  case CIPHER_des40:
    res->bulk_cipher_algorithm = DES;
    res->cipher_type = CIPHER_block;
    res->is_exportable = 1;
    res->key_material = 8;
    res->iv_size = 8;
    res->key_bits = 40;
    break;
  case CIPHER_null:
    res->bulk_cipher_algorithm = 0;
    res->cipher_type = CIPHER_stream;
    res->is_exportable = 1;
    res->key_material = 0;
    res->iv_size = 0;
    res->key_bits = 0;
    break;
  case CIPHER_rc4:
    res->bulk_cipher_algorithm = Crypto.Arcfour.State;
    res->cipher_type = CIPHER_stream;
    res->is_exportable = 0;
    res->key_material = 16;
    res->iv_size = 0;
    res->key_bits = 128;
    break;
  case CIPHER_des:
    res->bulk_cipher_algorithm = DES;
    res->cipher_type = CIPHER_block;
    res->is_exportable = 0;
    res->key_material = 8;
    res->iv_size = 8;
    res->key_bits = 56;
    break;
  case CIPHER_3des:
    res->bulk_cipher_algorithm = DES3;
    res->cipher_type = CIPHER_block;
    res->is_exportable = 0;
    res->key_material = 24;
    res->iv_size = 8;
    res->key_bits = 168;
    break;
#if constant(Crypto.IDEA)
  case CIPHER_idea:
    res->bulk_cipher_algorithm = Crypto.IDEA.CBC.Buffer.State;
    res->cipher_type = CIPHER_block;
    res->is_exportable = 0;
    res->key_material = 16;
    res->iv_size = 8;
    res->key_bits = 128;
    break;
#endif
  case CIPHER_aes:
    res->bulk_cipher_algorithm = Crypto.AES.CBC.Buffer.State;
    res->cipher_type = CIPHER_block;
    res->is_exportable = 0;
    res->key_material = 16;
    res->iv_size = 16;
    res->key_bits = 128;
    break;
  case CIPHER_aes256:
    res->bulk_cipher_algorithm = Crypto.AES.CBC.Buffer.State;
    res->cipher_type = CIPHER_block;
    res->is_exportable = 0;
    res->key_material = 32;
    res->iv_size = 16;
    res->key_bits = 256;
    break;
#if constant(Crypto.Camellia)
  case CIPHER_camellia128:
    res->bulk_cipher_algorithm = Crypto.Camellia.CBC.Buffer.State;
    res->cipher_type = CIPHER_block;
    res->is_exportable = 0;
    res->key_material = 16;
    res->iv_size = 16;
    res->key_bits = 128;
    break;
  case CIPHER_camellia256:
    res->bulk_cipher_algorithm = Crypto.Camellia.CBC.Buffer.State;
    res->cipher_type = CIPHER_block;
    res->is_exportable = 0;
    res->key_material = 32;
    res->iv_size = 16;
    res->key_bits = 256;
    break;
#endif
#if constant(Crypto.ChaCha20.POLY1305)
  case CIPHER_chacha20:
    if ((sizeof(algorithms) <= 3) || (algorithms[3] != MODE_poly1305)) {
      // Unsupported.
      return 0;
    }
    res->bulk_cipher_algorithm = Crypto.ChaCha20.POLY1305.State;
    res->cipher_type = CIPHER_aead;
    res->is_exportable = 0;
    res->key_material = 32;
    res->key_bits = 256;
    res->iv_size = 0;		// Length of the salt.
    res->explicit_iv_size = 0;	// Length of the explicit nonce/iv.
    res->hash_size = 0;		// No need for MAC keys.
    res->mac_algorithm = 0;	// MACs are not used with AEAD.

    break;
#endif
  default:
    return 0;
  }

  switch(algorithms[2])
  {
#if constant(Crypto.SHA512)
  case HASH_sha512:
    res->mac_algorithm = MAChmac_sha512;
    res->hash_size = 64;
    break;
#endif
#if constant(Crypto.SHA384)
  case HASH_sha384:
    res->mac_algorithm = MAChmac_sha384;
    res->hash_size = 48;
    break;
#endif
  case HASH_sha256:
    res->mac_algorithm = MAChmac_sha256;
    res->hash_size = 32;
    break;
  case HASH_sha1:
    if(version >= PROTOCOL_TLS_1_0)
      res->mac_algorithm = MAChmac_sha;
    else
      res->mac_algorithm = MACsha;
    res->hash_size = 20;
    break;
  case HASH_md5:
    if(version >= PROTOCOL_TLS_1_0)
      res->mac_algorithm = MAChmac_md5;
    else
      res->mac_algorithm = MACmd5;
    res->hash_size = 16;
    break;
  case 0:
    res->mac_algorithm = 0;
    res->hash_size = 0;
    break;
  default:
    return 0;
  }

  if ((version == PROTOCOL_SSL_3_0) && (ke_method != KE_rsa_fips)) {
    res->prf = prf_ssl_3_0;
  } else if (version <= PROTOCOL_TLS_1_1) {
    res->prf = prf_tls_1_0;
  } else {
    // The PRF is really part of the cipher suite in TLS 1.2.
    //
    // In all existing post TLS 1.2 cases the hash for the prf is the same
    // as the hash for the suite.
    switch(algorithms[2]) {
    case HASH_none:
      break;
    case HASH_md5:
    case HASH_sha1:
    case HASH_sha224:
      // These old suites are all upgraded to using SHA256.
    case HASH_sha256:
      res->prf = prf_tls_1_2;
      res->hash = Crypto.SHA256;
      break;
#if constant(Crypto.SHA384)
    case HASH_sha384:
      res->prf = prf_sha384;
      res->hash = Crypto.SHA384;
      break;
#endif
#if constant(Crypto.SHA384)
    case HASH_sha512:
      res->prf = prf_sha512;
      res->hash = Crypto.SHA512;
      break;
#endif
    default:
      return 0;
    }
  }

  if (sizeof(algorithms) > 3) {
    switch(algorithms[3]) {
    case MODE_cbc:
      break;
    case MODE_ccm:
      if (res->bulk_cipher_algorithm == Crypto.AES.CBC.Buffer.State) {
	res->bulk_cipher_algorithm = Crypto.AES.CCM.State;
      } else {
	// Unsupported.
	return 0;
      }
      res->cipher_type = CIPHER_aead;
      res->iv_size = 4;			// Length of the salt.
      res->explicit_iv_size = 8;	// Length of the explicit nonce/iv.
      res->hash_size = 0;		// No need for MAC keys.
      res->mac_algorithm = 0;		// MACs are not used with AEAD.

      break;
    case MODE_ccm_8:
      if (res->bulk_cipher_algorithm == Crypto.AES.CBC.Buffer.State) {
	res->bulk_cipher_algorithm = Crypto.AES.CCM8.State;
      } else {
	// Unsupported.
	return 0;
      }
      res->cipher_type = CIPHER_aead;
      res->iv_size = 4;			// Length of the salt.
      res->explicit_iv_size = 8;	// Length of the explicit nonce/iv.
      res->hash_size = 0;		// No need for MAC keys.
      res->mac_algorithm = 0;		// MACs are not used with AEAD.

      break;
#if constant(Crypto.AES.GCM)
    case MODE_gcm:
      if (res->bulk_cipher_algorithm == Crypto.AES.CBC.Buffer.State) {
	res->bulk_cipher_algorithm = Crypto.AES.GCM.State;
#if constant(Crypto.Camellia.GCM)
      } else if (res->bulk_cipher_algorithm ==
		 Crypto.Camellia.CBC.Buffer.State) {
	res->bulk_cipher_algorithm = Crypto.Camellia.GCM.State;
#endif
      } else {
	// Unsupported.
	return 0;
      }
      res->cipher_type = CIPHER_aead;
      res->iv_size = 4;			// Length of the salt.
      res->explicit_iv_size = 8;	// Length of the explicit nonce/iv.
      res->hash_size = 0;		// No need for MAC keys.
      res->mac_algorithm = 0;		// MACs are not used with AEAD.

      break;
#endif
#if constant(Crypto.ChaCha20.POLY1305)
    case MODE_poly1305:
      res->mac_algorithm = 0;		// MACs are not used with AEAD.
      res->hash_size = 0;
      break;
#endif
    default:
      return 0;
    }
  }

  if (version >= PROTOCOL_TLS_1_3) {
    // NB: Only DHE and ECDHE are supported in TLS 1.3 and later.
    switch(ke_method) {
    case KE_dh_anon:
    case KE_dhe_rsa:
    case KE_dhe_dss:
      res->ke_factory = KeyShareDHE;
      break;
#if constant(Crypto.ECC.Curve)
    case KE_ecdhe_rsa:
    case KE_ecdhe_ecdsa:
    case KE_ecdh_anon:
      res->ke_factory = KeyShareECDHE;
      break;
#endif
    default:
      werror( "%s: Unsupported KE: %s\n",
	      fmt_version(version),
	      fmt_constant(ke_method, "KE") );
      return 0;
    }
  } else {
    switch(ke_method)
    {
    case KE_null:
      res->ke_factory = KeyExchangeNULL;
      break;
    case KE_rsa:
    case KE_rsa_fips:
      res->ke_factory = KeyExchangeRSA;
      break;
    case KE_rsa_export:
      res->ke_factory = KeyExchangeExportRSA;
      break;
    case KE_dh_dss:
    case KE_dh_rsa:
      res->ke_factory = KeyExchangeDH;
      break;
    case KE_dh_anon:
    case KE_dhe_rsa:
    case KE_dhe_dss:
      res->ke_factory = KeyExchangeDHE;
      break;
#if constant(Crypto.ECC.Curve)
    case KE_ecdhe_rsa:
    case KE_ecdhe_ecdsa:
    case KE_ecdh_anon:
      res->ke_factory = KeyExchangeECDHE;
      break;
    case KE_ecdh_rsa:
    case KE_ecdh_ecdsa:
      res->ke_factory = KeyExchangeECDH;
      break;
    case KE_ecdhe_psk:
      res->ke_factory = KeyExchangeECDHEPSK;
      break;
#endif
    case KE_psk:
      res->ke_factory = KeyExchangePSK;
      break;
    case KE_dhe_psk:
      res->ke_factory = KeyExchangeDHEPSK;
      break;
    case KE_rsa_psk:
      res->ke_factory = KeyExchangeRSAPSK;
      break;
    default:
      error( "Internal error.\n" );
    }
  }

  switch(ke_method)
  {
  case KE_rsa:
  case KE_rsa_export:
  case KE_rsa_fips:
  case KE_dhe_rsa:
  case KE_ecdhe_rsa:
  case KE_rsa_psk:
    res->signature_alg = SIGNATURE_rsa;
    break;
  case KE_dh_rsa:
  case KE_dh_dss:
  case KE_dhe_dss:
    res->signature_alg = SIGNATURE_dsa;
    break;
  case KE_null:
  case KE_dh_anon:
  case KE_ecdh_anon:
  case KE_psk:
  case KE_dhe_psk:
#if constant(Crypto.ECC.Curve)
  case KE_ecdhe_psk:
#endif
    res->signature_alg = SIGNATURE_anonymous;
    break;
#if constant(Crypto.ECC.Curve)
  case KE_ecdh_rsa:
  case KE_ecdh_ecdsa:
  case KE_ecdhe_ecdsa:
    // FIXME: SIGNATURE_ecdsa used for KE_ecdh_rsa. Naming issue?
    res->signature_alg = SIGNATURE_ecdsa;
    break;
#endif
  default:
    error( "Internal error.\n" );
  }

  if (version >= PROTOCOL_TLS_1_2)
  {
    res->set_hash(max_hash_size, signature_algorithms);
  }

  if (res->is_exportable && res->bulk_cipher_algorithm &&
      (version >= PROTOCOL_TLS_1_1)) {
    // RFC 4346 A.5:
    // TLS 1.1 implementations MUST NOT negotiate
    // these cipher suites in TLS 1.1 mode.
    SSL3_DEBUG_MSG("Suite not supported in TLS 1.1 and later.\n");
    return 0;
  }

  if (version >= PROTOCOL_TLS_1_3) {
    if (res->cipher_type != CIPHER_aead) {
      // TLS1.3 6.1 RecordProtAlgorithm
      SSL3_DEBUG_MSG("Suite not supported in TLS 1.3 and later.\n");
      return 0;
    }
  }
  else if (version == PROTOCOL_TLS_1_2) {
    if (res->bulk_cipher_algorithm == DES) {
      // RFC 5246 1.2:
      // Removed IDEA and DES cipher suites.  They are now deprecated and
      // will be documented in a separate document.
      SSL3_DEBUG_MSG("Suite not supported in TLS 1.2 and later.\n");
      return 0;
    }
#if constant(Crypto.IDEA.CBC)
    if (res->bulk_cipher_algorithm == Crypto.IDEA.CBC.Buffer.State) {
      // RFC 5246 1.2:
      // Removed IDEA and DES cipher suites.  They are now deprecated and
      // will be documented in a separate document.
      SSL3_DEBUG_MSG("Suite not supported in TLS 1.2 and later.\n");
      return 0;
    }
#endif
  } else if (res->cipher_type == CIPHER_aead) {
    // RFC 5822 4:
    // These cipher suites make use of the authenticated encryption
    // with additional data defined in TLS 1.2 [RFC5246]. They MUST
    // NOT be negotiated in older versions of TLS.
    SSL3_DEBUG_MSG("Suite not supported prior to TLS 1.2.\n");
    return 0;
  }

  // Calculate a safe value for max_bytes.
  switch(res->cipher_type) {
  case CIPHER_block:
    /* CBC requires the keys to change well before 2^(block_size/2) blocks
     * have been sent (block_size in bits). cf https://sweet32.info/
     */
    object(Crypto.BlockCipher) alg =
      function_object(res->bulk_cipher_algorithm);
    if (!alg) {
      // Special case for the algorithms defined in this file.
      alg = ([
	DES: Crypto.DES,
	DES3: Crypto.DES,
#if constant(Crypto.Arctwo)
	RC2: Crypto.Arctwo,
#endif
      ])[res->bulk_cipher_algorithm];
      if (!alg) {
	error("Failed to determine algorithm for %O\n",
	      res->bulk_cipher_algorithm);
      }
    }
    int block_size = alg->block_size();
    /* The strict limit is thus block_size << block_size * 4 (block_size in bytes).
     * We want some safety margin and multiply the exponent with 3 instead of 4.
     */
    res->max_bytes = block_size << block_size*3;
    break;
  case CIPHER_stream:
    /* Currently RC4 is the only stream cipher.
     * We set the rekey threshold to the same as for DES/DES3.
     */
    res->max_bytes = 0x08000000;
    break;
  case CIPHER_aead:
    // All the AEAD modes should handle 2GB without any problem.
    res->max_bytes = 0x7fffffff;
    break;
  }
  if (res->is_exportable) res->max_bytes >>= 4;
  if (res->max_bytes > 0x7fffffff) res->max_bytes = 0x7fffffff;

  return res;
}
