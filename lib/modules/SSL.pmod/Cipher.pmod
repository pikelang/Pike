#pike __REAL_VERSION__
#require constant(Crypto.Hash)

//! Encryption and MAC algorithms used in SSL.

import .Constants;

#ifdef SSL3_DEBUG
#define SSL3_DEBUG_MSG(X ...)  werror(X)
#else /*! SSL3_DEBUG */
#define SSL3_DEBUG_MSG(X ...)
#endif /* SSL3_DEBUG */

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
  //! @param seq_num
  //!   Sequence number for the packet in the stream.
  //! @adjust_len
  //!   Added to @tt{sizeof(packet)@} to get the packet length.
  //! @returns
  //!   Returns the MAC hash for the @[packet].
  string hash_packet(object packet, int seq_num, int|void adjust_len);

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
  //! This is used by AEAD ciphers, where there's a secret part of the iv
  //! "salt" of length @[iv_size], and an explicit part that is sent in
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
  HashAlgorithm signature_hash = HASH_sha;

  //! The signature algorithm used for key exchange signatures.
  SignatureAlgorithm signature_alg;

  //! The function used to sign packets.
  ADT.struct sign(object session, string(8bit) cookie, ADT.struct struct)
  {
    if( signature_alg == SIGNATURE_anonymous )
      return struct;

    // RFC 5246 4.7
    if( session->version >= PROTOCOL_TLS_1_2 )
    {
      string sign =
        session->private_key->pkcs_sign(cookie + struct->contents(),
                                        HASH_lookup[signature_hash]);
      struct->put_uint(signature_hash, 1);
      struct->put_uint(signature_alg, 1);
      struct->put_var_string(sign, 2);
      return struct;
    }

    // RFC 4346 7.4.3 (struct Signature)
    string data = cookie + struct->contents();
    switch( signature_alg )
    {
    case SIGNATURE_rsa:
      {
        string digest = Crypto.MD5->hash(data) + Crypto.SHA1->hash(data);
        object s = session->private_key->raw_sign(digest);
        struct->put_bignum(s);
        return struct;
      }

    case SIGNATURE_dsa:
    case SIGNATURE_ecdsa:
      {
        string s =
          session->private_key->pkcs_sign(cookie + struct->contents(),
                                          Crypto.SHA1);
        struct->put_var_string(s, 2);
        return struct;
      }
    }

    error("Internal error");
  }

  //! The function used to verify the signature for packets.
  int(0..1) verify(object session, string cookie, ADT.struct struct,
                   ADT.struct input)
  {
    if( signature_alg == SIGNATURE_anonymous )
      return 1;

    // RFC 5246 4.7
    if( session->version >= PROTOCOL_TLS_1_2 )
    {
      int hash_id = input->get_uint(1);
      int sign_id = input->get_uint(1);
      string sign = input->get_var_string(2);

      Crypto.Hash hash = HASH_lookup[hash_id];
      if (!hash) return 0;

      // FIXME: Validate that the sign_id is correct.

      return session->peer_public_key &&
        session->peer_public_key->pkcs_verify(cookie + struct->contents(),
                                              hash, sign);
    }

    // RFC 4346 7.4.3 (struct Signature)
    string data = cookie + struct->contents();
    switch( signature_alg )
    {
    case SIGNATURE_rsa:
      {
        string digest = Crypto.MD5->hash(data) + Crypto.SHA1->hash(data);
        Gmp.mpz signature = input->get_bignum();
        return session->peer_public_key->raw_verify(digest, signature);
      }

    case SIGNATURE_dsa:
    case SIGNATURE_ecdsa:
      {
        return session->peer_public_key->
          pkcs_verify(data, Crypto.SHA1, input->get_var_string(2));
      }
    }

    error("Internal error");
  }

  void set_hash(int max_hash_size,
                array(array(int)) signature_algorithms,
                int wanted_hash_id)
  {
    // Stay with SHA1 for requests without signature algorithms
    // extensions (RFC 5246 7.4.1.4.1) and anonymous requests.
    if( signature_alg == SIGNATURE_anonymous || !signature_algorithms )
      return;

    int hash_id = -1;
    SSL3_DEBUG_MSG("Signature algorithms (max hash size %d):\n%s",
                   max_hash_size, fmt_signature_pairs(signature_algorithms));
    foreach(signature_algorithms, array(int) pair) {
      if ((pair[1] == signature_alg) && HASH_lookup[pair[0]]) {
        if (pair[0] == wanted_hash_id) {
          // Use the required hash from Suite B if available.
          hash_id = wanted_hash_id;
          break;
        }
        if (max_hash_size < HASH_lookup[pair[0]]->digest_size()) {
          // Eg RSA has a maximum block size and the digest is too large.
          continue;
        }
        if (pair[0] > hash_id) {
          hash_id = pair[0];
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
  //! Indicates whether a certificate isn't required.
  int anonymous;

  //! Indicates whether the key exchange has failed due to bad MACs.
  int message_was_bad;

  //! @returns
  //!   Returns an @[ADT.struct] with the
  //!   @[HANDSHAKE_server_key_exchange] payload.
  ADT.struct server_key_params();

  //! The default implementation calls @[server_key_params()] to generate
  //! the base payload.
  //!
  //! @returns
  //!   Returns the signed payload for a @[HANDSHAKE_server_key_exchange].
  string server_key_exchange_packet(string client_random, string server_random)
  {
    ADT.struct struct = server_key_params();
    if (!struct) return 0;

    session->cipher_spec->sign(session, client_random + server_random, struct);
    return struct->pop_data();
  }

  //! Derive the master secret from the premaster_secret
  //! and the random seeds.
  //!
  //! @returns
  //!   Returns the master secret.
  string derive_master_secret(string premaster_secret,
			      string client_random,
			      string server_random,
			      ProtocolVersion version)
  {
    string res = "";

    SSL3_DEBUG_MSG("KeyExchange: in derive_master_secret is version=0x%x\n",
		   version);

    res = session->cipher_spec->prf(premaster_secret, "master secret",
				    client_random + server_random, 48);

    connection->ke = UNDEFINED;

    SSL3_DEBUG_MSG("master: %O\n", res);
    return res;
  }

  //! @returns
  //!   Returns the payload for a @[HANDSHAKE_client_key_exchange] packet.
  //!   May return @expr{0@} (zero) to generate an @[ALERT_unexpected_message].
  string client_key_exchange_packet(string client_random,
				    string server_random,
				    ProtocolVersion version);

  //! @param data
  //!   Payload from a @[HANDSHAKE_client_key_exchange].
  //!
  //! @returns
  //!   Master secret or alert number.
  //!
  //! @note
  //!   May set @[message_was_bad] and return a fake master secret.
  string(8bit)|int(8bit) server_derive_master_secret(string(8bit) data,
						 string(8bit) client_random,
						 string(8bit) server_random,
						 ProtocolVersion version);

  //! @param input
  //!   @[ADT.struct] with the content of a @[HANDSHAKE_server_key_exchange].
  //!
  //! @returns
  //!   The key exchange information should be extracted from @[input],
  //!   so that it is positioned at the signature.
  //!
  //!   Returns a new @[ADT.struct] with the unsigned payload of @[input].
  ADT.struct parse_server_key_exchange(ADT.struct input);

  //! @param input
  //!   @[ADT.struct] with the content of a @[HANDSHAKE_server_key_exchange].
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
  int server_key_exchange(ADT.struct input,
			  string client_random,
			  string server_random)
  {
    SSL3_DEBUG_MSG("SSL.Session: SERVER_KEY_EXCHANGE\n");

    ADT.struct temp_struct = parse_server_key_exchange(input);

    int verification_ok;
    mixed err = catch {
	verification_ok = session->cipher_spec->verify(
          session, client_random + server_random, temp_struct, input);
      };
#ifdef SSL3_DEBUG
    if( err ) {
      master()->handle_error(err);
    }
#endif
    err = UNDEFINED;
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

  ADT.struct server_key_params()
  {
    return ADT.struct();
  }

  string client_key_exchange_packet(string client_random,
				    string server_random,
				    ProtocolVersion version)
  {
    anonymous = 1;
    session->master_secret = "";
    return "";
  }

  //! @returns
  //!   Master secret or alert number.
  string(8bit) server_derive_master_secret(string(8bit) data,
                                           string(8bit) client_random,
                                           string(8bit) server_random,
                                           ProtocolVersion version)
  {
    anonymous = 1;
    return "";
  }

  int server_key_exchange(ADT.struct input,
			  string client_random,
			  string server_random)
  {
    return 0;
  }
}

//! Key exchange for @[KE_rsa].
//!
//! @[KeyExchange] that uses the Rivest Shamir Adelman algorithm.
class KeyExchangeRSA
{
  inherit KeyExchange;

  Crypto.RSA temp_key; /* Key used for session key exchange (if not the same
			* as the server's certified key) */

  ADT.struct server_key_params()
  {
    ADT.struct struct;
  
    SSL3_DEBUG_MSG("KE_RSA\n");
    if (session->cipher_spec->is_exportable)
    {
      /* Send a ServerKeyExchange message. */

      if (temp_key = context->short_rsa) {
	if (--context->short_rsa_counter <= 0) {
	  context->short_rsa = UNDEFINED;
	}
      } else {
	// RFC 2246 D.1:
	// For typical electronic commerce applications, it is suggested
	// that keys be changed daily or every 500 transactions, and more
	// often if possible.

	// Now >10 years later, an aging laptop is capable of generating
	// >75 512-bit RSA keys per second, and clients requiring
	// export suites are getting far between, so rotating the key
	// after 5 uses seems ok. When the key generation rate reaches
	// ~250/second, I believe rotating the key every transaction
	// will be feasible.
	//	/grubba 2014-03-23
	SSL3_DEBUG_MSG("Generating a new ephemeral 512-bit RSA key.\n");
	context->short_rsa_counter = 5 - 1;
	context->short_rsa = temp_key = Crypto.RSA()->generate_key(512);
      }

      SSL3_DEBUG_MSG("Sending a server key exchange-message, "
		     "with a %d-bits key.\n", temp_key->key_size());
      struct = ADT.struct();
      struct->put_bignum(temp_key->get_n());
      struct->put_bignum(temp_key->get_e());
    }
    else
      return 0;

    return struct;
  }

  string client_key_exchange_packet(string client_random,
				    string server_random,
				    ProtocolVersion version)
  {
    SSL3_DEBUG_MSG("client_key_exchange_packet(%O, %O, %d.%d)\n",
		   client_random, server_random, version>>8, version & 0xff);
    ADT.struct struct = ADT.struct();
    string data;
    string premaster_secret;

    SSL3_DEBUG_MSG("KE_RSA\n");
    // NOTE: To protect against version roll-back attacks,
    //       the version sent here MUST be the same as the
    //       one in the initial handshake!
    struct->put_uint(client_version, 2);
    struct->put_fix_string( context->random(46) );
    premaster_secret = struct->pop_data();

    SSL3_DEBUG_MSG("temp_key: %O\n"
		   "session->peer_public_key: %O\n",
		   temp_key, session->peer_public_key);
    data = (temp_key || session->peer_public_key)->encrypt(premaster_secret);

    if(version >= PROTOCOL_TLS_1_0)
      data = sprintf("%2H", [string(8bit)]data);

    session->master_secret = derive_master_secret(premaster_secret,
						  client_random,
						  server_random,
						  version);
    return data;
  }

  //! @returns
  //!   Master secret or alert number.
  string(8bit)|int(8bit) server_derive_master_secret(string(8bit) data,
                                                     string(8bit) client_random,
                                                     string(8bit) server_random,
                                                     ProtocolVersion version)
  {
    string premaster_secret;

    SSL3_DEBUG_MSG("server_derive_master_secret: ke_method %d\n",
		   session->ke_method);

    SSL3_DEBUG_MSG("KE_RSA\n");
    /* Decrypt the premaster_secret */
    SSL3_DEBUG_MSG("encrypted premaster_secret: %O\n", data);
    SSL3_DEBUG_MSG("temp_key: %O\n"
		   "session->private_key: %O\n",
		   temp_key, session->private_key);
    string rest = "";
    if(version >= PROTOCOL_TLS_1_0)
      sscanf(data, "%2H%s", data, rest);
    // Decrypt, even when we know data is incorrect, for time invariance.
    premaster_secret = (temp_key || session->private_key)->decrypt(data);

    SSL3_DEBUG_MSG("premaster_secret: %O\n", premaster_secret);

    // We want both branches to execute in equal time (ignoring
    // SSL3_DEBUG in the hope it is never on in production).
    // Workaround documented in RFC 2246.
    if ( `+( !premaster_secret, sizeof(rest),
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
      werror("SSL.ServerConnection: Invalid premaster_secret! "
	     "A chosen ciphertext attack?\n");
      if (premaster_secret && sizeof(premaster_secret) > 2) {
	werror("SSL.ServerConnection: Strange version (%d.%d) detected in "
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

    return derive_master_secret(premaster_secret, client_random, server_random,
				version);
  }

  ADT.struct parse_server_key_exchange(ADT.struct input)
  {
    ADT.struct temp_struct = ADT.struct();

    SSL3_DEBUG_MSG("KE_RSA\n");
    Gmp.mpz n = input->get_bignum();
    Gmp.mpz e = input->get_bignum();
    temp_struct->put_bignum(n);
    temp_struct->put_bignum(e);
    if (session->cipher_spec->is_exportable) {
      temp_key = Crypto.RSA()->set_public_key(n, e);
    }

    return temp_struct;
  }
}

//! Key exchange for @[KE_dh_dss] and @[KE_dh_dss].
//!
//! @[KeyExchange] that uses Diffie-Hellman with a key from
//! a DSS certificate.
class KeyExchangeDH
{
  inherit KeyExchange;

  .Cipher.DHKeyExchange dh_state; /* For diffie-hellman key exchange */

  ADT.struct server_key_params()
  {
    ADT.struct struct;

    SSL3_DEBUG_MSG("KE_DH\n");
    // anonymous, not used on the server, but here for completeness.
    anonymous = 1;
    struct = ADT.struct();

    dh_state =
      .Cipher.DHKeyExchange(Crypto.DH.Parameters(session->private_key));
    dh_state->secret = session->private_key->get_x();

    // RFC 4346 7.4.3:
    // It is not legal to send the server key exchange message for the
    // following key exchange methods:
    //
    //   RSA
    //   DH_DSS
    //   DH_RSA
    return 0;
  }

  string client_key_exchange_packet(string client_random,
				    string server_random,
				    ProtocolVersion version)
  {
    SSL3_DEBUG_MSG("client_key_exchange_packet(%O, %O, %d.%d)\n",
		   client_random, server_random, version>>8, version & 0xff);
    ADT.struct struct = ADT.struct();
    string data;
    string premaster_secret;

    SSL3_DEBUG_MSG("KE_DH\n");
    anonymous = 1;
    if (!dh_state) {
      dh_state =
	.Cipher.DHKeyExchange(Crypto.DH.Parameters(session->peer_public_key));
      dh_state->other = session->peer_public_key->get_y();
    }
    dh_state->new_secret(context->random);
    struct->put_bignum(dh_state->our);
    data = struct->pop_data();
    premaster_secret = dh_state->get_shared()->digits(256);

    session->master_secret =
      derive_master_secret(premaster_secret, client_random, server_random,
			   version);
    return data;
  }

  //! @returns
  //!   Master secret or alert number.
  string(8bit)|int(8bit) server_derive_master_secret(string(8bit) data,
						 string(8bit) client_random,
						 string(8bit) server_random,
						 ProtocolVersion version)
  {
    string premaster_secret;

    SSL3_DEBUG_MSG("server_derive_master_secret: ke_method %d\n",
		   session->ke_method);

    SSL3_DEBUG_MSG("KE_DH\n");
    anonymous = 1;

    /* Explicit encoding */
    ADT.struct struct = ADT.struct(data);

    if (catch
      {
	dh_state->set_other(struct->get_bignum());
      } || sizeof(struct))
    {
      connection->ke = UNDEFINED;
      return ALERT_unexpected_message;
    }

    premaster_secret = dh_state->get_shared()->digits(256);
    dh_state = 0;

    return derive_master_secret(premaster_secret, client_random, server_random,
				version);
  }

  ADT.struct parse_server_key_exchange(ADT.struct input)
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

//! KeyExchange for @[KE_dhe_rsa], @[KE_dhe_dss] and @[KE_dh_anon].
//!
//! KeyExchange that uses Diffie-Hellman to generate an Ephemeral key.
class KeyExchangeDHE
{
  inherit KeyExchangeDH;

  ADT.struct server_key_params()
  {
    ADT.struct struct;

    SSL3_DEBUG_MSG("KE_DHE\n");
    // anonymous, not used on the server, but here for completeness.
    anonymous = 1;
    struct = ADT.struct();

    // Default to using MODP Group 24 (2048/256 bits).
    dh_state = .Cipher.DHKeyExchange(Crypto.DH.MODPGroup24);
    dh_state->new_secret(context->random);

    struct->put_bignum(dh_state->parameters->p);
    struct->put_bignum(dh_state->parameters->g);
    struct->put_bignum(dh_state->our);

    return struct;
  }

  //! @returns
  //!   Master secret or alert number.
  string(8bit)|int(8bit) server_derive_master_secret(string(8bit) data,
						 string(8bit) client_random,
						 string(8bit) server_random,
						 ProtocolVersion version)
  {
    SSL3_DEBUG_MSG("server_derive_master_secret: ke_method %d\n",
		   session->ke_method);

    SSL3_DEBUG_MSG("KE_DHE\n");

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

    return ::server_derive_master_secret(data, client_random, server_random,
					 version);
  }

  ADT.struct parse_server_key_exchange(ADT.struct input)
  {
    ADT.struct temp_struct = ADT.struct();

    SSL3_DEBUG_MSG("KE_DHE\n");
    Gmp.mpz p = input->get_bignum();
    Gmp.mpz g = input->get_bignum();
    Gmp.mpz order = [object(Gmp.mpz)]((p-1)/2); // FIXME: Is this correct?
    temp_struct->put_bignum(p);
    temp_struct->put_bignum(g);
    dh_state = .Cipher.DHKeyExchange(Crypto.DH.Parameters(p, g, order));
    dh_state->set_other(input->get_bignum());
    temp_struct->put_bignum(dh_state->other);

    return temp_struct;
  }
}

#if constant(Crypto.ECC.Curve)
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
  inherit KeyExchange;

  string(8bit) encode_point(Gmp.mpz x, Gmp.mpz y)
  {
    // ANSI x9.62 4.3.6.
    // Format #4 is uncompressed.
    return sprintf("%c%*c%*c",
		   4,
		   (session->curve->size() + 7)>>3, x,
		   (session->curve->size() + 7)>>3, y);
  }

  array(Gmp.mpz) decode_point(string(8bit) data)
  {
    ADT.struct struct = ADT.struct(data);

    Gmp.mpz x;
    Gmp.mpz y;
    switch(struct->get_uint(1)) {
    case 4:
      string rest = struct->get_rest();
      if (sizeof(rest) & 1) {
	connection->ke = UNDEFINED;
	error("Invalid size in point format.\n");
      }
      [x, y] = map(rest/(sizeof(rest)/2), Gmp.mpz, 256);
      break;
    default:
      // Compressed points not supported yet.
      connection->ke = UNDEFINED;
      error("Unsupported point format.\n");
      break;
    }
    return ({ x, y });
  }

  protected Gmp.mpz get_server_secret()
  {
    return session->private_key->get_private_key();
  }

  protected Gmp.mpz get_server_pubx()
  {
    return session->peer_public_key->get_x();
  }

  protected Gmp.mpz get_server_puby()
  {
    return session->peer_public_key->get_y();
  }

  ADT.struct server_key_params()
  {
    SSL3_DEBUG_MSG("KE_ECDH\n");
    // RFC 4492 2.1:
    // A ServerKeyExchange MUST NOT be sent (the server's certificate
    // contains all the necessary keying information required by the client
    // to arrive at the premaster secret).
    return 0;
  }

  string client_key_exchange_packet(string client_random,
				    string server_random,
				    ProtocolVersion version)
  {
    SSL3_DEBUG_MSG("client_key_exchange_packet(%O, %O, %d.%d)\n",
		   client_random, server_random, version>>8, version & 0xff);
    ADT.struct struct = ADT.struct();
    string data;
    string premaster_secret;

    SSL3_DEBUG_MSG("KE_ECDH\n");

    Gmp.mpz secret = session->curve->new_scalar(context->random);
    [Gmp.mpz x, Gmp.mpz y] = session->curve * secret;

    ADT.struct point = ADT.struct();

    struct->put_var_string(encode_point(x, y), 1);

    data = struct->pop_data();

    Gmp.mpz pubx = get_server_pubx();
    Gmp.mpz puby = get_server_puby();

    // RFC 4492 5.10:
    // Note that this octet string (Z in IEEE 1363 terminology) as
    // output by FE2OSP, the Field Element to Octet String
    // Conversion Primitive, has constant length for any given
    // field; leading zeros found in this octet string MUST NOT be
    // truncated.
    premaster_secret =
      sprintf("%*c",
	      (session->curve->size() + 7)>>3,
	      session->curve->point_mul(pubx, puby, secret)[0]);

    secret = 0;

    session->master_secret =
      derive_master_secret(premaster_secret, client_random, server_random,
			   version);
    return data;
  }

  //! @returns
  //!   Master secret or alert number.
  string(8bit)|int(8bit) server_derive_master_secret(string(8bit) data,
						 string(8bit) client_random,
						 string(8bit) server_random,
						 ProtocolVersion version)
  {
    SSL3_DEBUG_MSG("server_derive_master_secret: ke_method %d\n",
		   session->ke_method);

    SSL3_DEBUG_MSG("KE_ECDH\n");

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

    /* Explicit encoding */
    ADT.struct struct = ADT.struct(data);

    if (catch
      {
	[ Gmp.mpz x, Gmp.mpz y ] = decode_point(struct->get_var_string(1));
	Gmp.mpz secret = get_server_secret();
	// RFC 4492 5.10:
	// Note that this octet string (Z in IEEE 1363 terminology) as
	// output by FE2OSP, the Field Element to Octet String
	// Conversion Primitive, has constant length for any given
	// field; leading zeros found in this octet string MUST NOT be
	// truncated.
	premaster_secret =
	  sprintf("%*c",
		  (session->curve->size() + 7)>>3,
		  session->curve->point_mul(x, y, secret)[0]);
      } || sizeof(struct))
    {
      connection->ke = UNDEFINED;
      return ALERT_unexpected_message;
    }

    return derive_master_secret(premaster_secret, client_random, server_random,
				version);
  }

  ADT.struct parse_server_key_exchange(ADT.struct input)
  {
    SSL3_DEBUG_MSG("KE_ECDH\n");
    // RFC 4492 2.1:
    // A ServerKeyExchange MUST NOT be sent (the server's certificate
    // contains all the necessary keying information required by the client
    // to arrive at the premaster secret).
    error("Invalid message.\n");
  }
}

//! KeyExchange for @[KE_ecdhe_rsa] and @[KE_ecdhe_ecdsa].
//!
//! KeyExchange that uses Elliptic Curve Diffie-Hellman to
//! generate an Ephemeral key.
class KeyExchangeECDHE
{
  inherit KeyExchangeECDH;

  protected Gmp.mpz secret;
  protected Gmp.mpz pubx;
  protected Gmp.mpz puby;

  protected Gmp.mpz get_server_secret()
  {
    return secret;
  }

  protected Gmp.mpz get_server_pubx()
  {
    return pubx;
  }

  protected Gmp.mpz get_server_puby()
  {
    return puby;
  }

  ADT.struct server_key_params()
  {
    ADT.struct struct;

    SSL3_DEBUG_MSG("KE_ECDHE\n");
    // anonymous, not used on the server, but here for completeness.
    anonymous = 1;
    struct = ADT.struct();

    // Select a suitable curve.
    int c;
    switch(session->cipher_spec->key_bits) {
    case 257..:
      c = CURVE_secp521r1;
      break;
    case 129..256:
      // Suite B requires SECP384r1/SHA256 for AES-256
      c = CURVE_secp384r1;
      break;
    case ..128:
      // Suite B requires SECP256r1/SHA256 for AES-128
      c = CURVE_secp256r1;
      break;
    }

    if (!has_value(session->ecc_curves, c)) {
      // Preferred curve not available -- Select the strongest available.
      c = session->ecc_curves[0];
    }
    session->curve = ECC_CURVES[c];

    SSL3_DEBUG_MSG("Curve: %O (%O)\n", session->curve, c);

    secret = session->curve->new_scalar(context->random);
    [Gmp.mpz x, Gmp.mpz y] = session->curve * secret;

    SSL3_DEBUG_MSG("secret: %O\n", secret);
    SSL3_DEBUG_MSG("x: %O\n", x);
    SSL3_DEBUG_MSG("y: %O\n", y);

    struct->put_uint(CURVETYPE_named_curve, 1);
    struct->put_uint(c, 2);

    struct->put_var_string(encode_point(x, y), 1);

    return struct;
  }

  string client_key_exchange_packet(string client_random,
				    string server_random,
				    ProtocolVersion version)
  {
    anonymous = 1;
    return ::client_key_exchange_packet(client_random, server_random, version);
  }

  //! @returns
  //!   Master secret or alert number.
  string(8bit)|int(8bit) server_derive_master_secret(string(8bit) data,
						 string(8bit) client_random,
						 string(8bit) server_random,
						 ProtocolVersion version)
  {
    anonymous = 1;
    return ::server_derive_master_secret(data, client_random,
					 server_random, version);
  }

  ADT.struct parse_server_key_exchange(ADT.struct input)
  {
    SSL3_DEBUG_MSG("KE_ECDHE\n");

    ADT.struct temp_struct = ADT.struct();

    // First the curve.
    switch(input->get_uint(1)) {
    case CURVETYPE_named_curve:
      temp_struct->put_uint(CURVETYPE_named_curve, 1);
      int c = input->get_uint(2);
      temp_struct->put_uint(c, 2);
      session->curve = ECC_CURVES[c];
      if (!session->curve) {
	connection->ke = UNDEFINED;
	error("Unsupported curve.\n");
      }
      SSL3_DEBUG_MSG("Curve: %O (%O)\n", session->curve, c);
      break;
    default:
      connection->ke = UNDEFINED;
      error("Invalid curve encoding.\n");
      break;
    }

    // Then the point.
    string raw;
    [ pubx, puby ] = decode_point(raw = input->get_var_string(1));
    temp_struct->put_var_string(raw, 1);

    return temp_struct;
  }
}

#endif /* Crypto.ECC.Curve */

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

  string(8bit) hash_packet(object packet, int seq_num, int|void adjust_len)
  {
    string s = sprintf("%8c%c%2c", seq_num,
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

  string hash_packet(object packet, int seq_num, int|void adjust_len)
  {
    hmac->update( sprintf("%8c%c%2c%2c", seq_num,
                          packet->content_type,
                          packet->protocol_version,
                          sizeof(packet->fragment) + adjust_len));
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

//! Implements Diffie-Hellman key-exchange.
//!
//! The following key exchange methods are implemented here:
//! @[KE_dhe_dss], @[KE_dhe_rsa] and @[KE_dh_anon].
class DHKeyExchange
{
  /* Public parameters */
  Crypto.DH.Parameters parameters;

  Gmp.mpz our; /* Our value */
  Gmp.mpz other; /* Other party's value */
  Gmp.mpz secret; /* our =  g ^ secret mod p */

  //!
  protected void create(Crypto.DH.Parameters p) {
    parameters = p;
  }

  this_program new_secret(function random)
  {
    [our, secret] = parameters->generate_keypair(random);
    return this;
  }
  
  this_program set_other(Gmp.mpz o) {
    other = o;
    return this;
  }

  Gmp.mpz get_shared() {
    return other->powm(secret, parameters->p);
  }
}

//! Lookup the crypto parameters for a cipher suite.
//!
//! @param suite
//!   Cipher suite to lookup.
//!
//! @param version
//!   Version of the SSL/TLS protocol to support.
//!
//! @param signature_algorithms
//!   The set of <hash, signature> combinations that
//!   are supported by the other end.
//!
//! @param max_hash_size
//!   The maximum hash size supported for the signature algorithm.
//!
//! @returns
//!   Returns @expr{0@} (zero) for unsupported combinations, otherwise
//!   returns an initialized @[CipherSpec] for the @[suite].
CipherSpec lookup(int suite, ProtocolVersion|int version,
                  array(array(int)) signature_algorithms,
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
#if constant(Crypto.ChaCha20)
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
  case HASH_sha:
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
    case HASH_sha:
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

  switch(ke_method)
  {
  case KE_null:
    res->ke_factory = KeyExchangeNULL;
    break;
  case KE_rsa:
  case KE_rsa_fips:
    res->ke_factory = KeyExchangeRSA;
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
#endif
  default:
    error( "Internal error.\n" );
  }

  switch(ke_method)
  {
  case KE_rsa:
  case KE_rsa_fips:
  case KE_dhe_rsa:
  case KE_ecdhe_rsa:
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
    int wanted_hash_id;
    if( res->signature_alg == SIGNATURE_ecdsa )
    {
      if (res->key_bits < 256) {
	// Suite B requires SHA256 with AES-128.
	wanted_hash_id = HASH_sha256;
      } else if (res->key_bits < 384) {
	// Suite B requires SHA384 with AES-256.
	wanted_hash_id = HASH_sha384;
      }
    }

    res->set_hash(max_hash_size, signature_algorithms, wanted_hash_id);
  }

  if (res->is_exportable && (version >= PROTOCOL_TLS_1_1)) {
    // RFC 4346 A.5:
    // TLS 1.1 implementations MUST NOT negotiate
    // these cipher suites in TLS 1.1 mode.
    SSL3_DEBUG_MSG("Suite not supported in TLS 1.1 and later.\n");
    return 0;
  }

  if (version >= PROTOCOL_TLS_1_2) {
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

  return res;
}
