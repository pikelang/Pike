#pike __REAL_VERSION__

#if constant(Crypto.Hash)

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
  optional string unpad(string);
  optional string pad();

  optional this_program set_iv(string);
}

//! Message Authentication Code interface.
class MACAlgorithm {
  //! @param packet
  //!   @[Packet] to generate a MAC hash for.
  //! @param seq_num
  //!   Sequence number for the packet in the stream.
  //! @returns
  //!   Returns the MAC hash for the @[packet].
  string hash(object packet, Gmp.mpz seq_num);

  //! Hashes the data with the hash algorithm and retuns it as a raw
  //! binary string.
  string hash_raw(string data);

  //! The length of the header prefixed by @[hash()].
  constant hash_header_size = 13;
}

//! Cipher specification.
class CipherSpec {
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

  //! The effective number of bits in @[key_material].
  //!
  //! This is typically @expr{key_material * 8@}, but for eg @[DES]
  //! this is @expr{key_material * 7@}.
  int key_bits;

  //! The function used to sign packets.
  function(object,string,ADT.struct:ADT.struct) sign;

  //! The function used to verify the signature for packets.
  function(object,string,ADT.struct,Gmp.mpz:int(0..1)) verify;
}

//! KeyExchange method.
class KeyExchange(object context, object session, array(int) client_version)
{
  Crypto.RSA temp_key; /* Key used for session key exchange (if not the same
			* as the server's certified key) */

  .Cipher.DHKeyExchange dh_state; /* For diffie-hellman key exchange */

  int anonymous;

  int message_was_bad;

  string server_key_exchange_packet(string server_random, string client_random)
  {
    ADT.struct struct;
  
    switch (session->ke_method)
    {
    case KE_rsa:
      SSL3_DEBUG_MSG("KE_RSA\n");
      temp_key = (session->cipher_spec->is_exportable
		  ? context->short_rsa
		  : context->long_rsa);
      if (temp_key)
      {
	/* Send a ServerKeyExchange message. */

	SSL3_DEBUG_MSG("Sending a server key exchange-message, "
		       "with a %d-bits key.\n", temp_key->key_size());
	struct = ADT.struct();
	struct->put_bignum(temp_key->get_n());
	struct->put_bignum(temp_key->get_e());
      }
      else
	return 0;
      break;
    case KE_dhe_dss:
    case KE_dhe_rsa:
    case KE_dh_anon:
      SSL3_DEBUG_MSG("KE_DHE\n");
      // anonymous, not used on the server, but here for completeness.
      anonymous = 1;
      struct = ADT.struct();

      dh_state = context->dh_ke = .Cipher.DHKeyExchange(.Cipher.DHParameters());
      dh_state->new_secret(context->random);

      struct->put_bignum(dh_state->parameters->p);
      struct->put_bignum(dh_state->parameters->g);
      struct->put_bignum(dh_state->our);
      break;
    default:
      return 0;
    }

    session->cipher_spec->sign(session, client_random + server_random, struct);
    return struct->pop_data();
  }

  string client_derive_master_secret(string premaster_secret, string server_random, string client_random, array(int) version)
  {
    string res = "";

    .Cipher.MACsha sha = .Cipher.MACsha();
    .Cipher.MACmd5 md5 = .Cipher.MACmd5();

    SSL3_DEBUG_MSG("Handshake.pike: in client_derive_master_secret is version[1]="+version[1]+"\n");

    if(version[1] == PROTOCOL_SSL_3_0) {
      foreach( ({ "A", "BB", "CCC" }), string cookie)
	res += md5->hash_raw(premaster_secret
			     + sha->hash_raw(cookie + premaster_secret 
					     + client_random + server_random));
    }
    else if(version[1] >= PROTOCOL_TLS_1_0) {
      res+=.Cipher.prf(premaster_secret,"master secret",client_random+server_random,48);
    }

    SSL3_DEBUG_MSG("bahmaster: %O\n", res);
    return res;
  }

  string client_key_exchange_packet(string server_random, string client_random,
				    array(int) version)
  {
    SSL3_DEBUG_MSG("client_key_exchange_packet(%O, %O, %d.%d)\n",
		   server_random, client_random, version[0], version[1]);
    ADT.struct struct = ADT.struct();
    string data;
    string premaster_secret;

    switch (session->ke_method)
    {
    case KE_rsa:
      SSL3_DEBUG_MSG("KE_RSA\n");
      // NOTE: To protect against version roll-back attacks,
      //       the version sent here MUST be the same as the
      //       one in the initial handshake!
      struct->put_uint(client_version[0], 1);
      struct->put_uint(client_version[1], 1);
      string random = context->random(46);
      struct->put_fix_string(random);
      premaster_secret = struct->pop_data();

      SSL3_DEBUG_MSG("temp_key: %O\n"
		     "session->rsa: %O\n", temp_key, session->rsa);
      data = (temp_key || session->rsa)->encrypt(premaster_secret);

      if(version[1] >= PROTOCOL_TLS_1_0)
	data=sprintf("%2H", [string(0..255)]data);
      break;
    case KE_dhe_dss:
    case KE_dhe_rsa:
    case KE_dh_anon:
      SSL3_DEBUG_MSG("KE_DHE\n");
      anonymous = 1;
      context->dh_ke->new_secret(context->random);
      struct->put_bignum(context->dh_ke->our);
      data = struct->pop_data();
      premaster_secret = context->dh_ke->get_shared()->digits(256);
      break;
    default:
      return 0;
    }
    session->master_secret = client_derive_master_secret(premaster_secret,
							 server_random,
							 client_random,
							 version);
    return data;
  }

  //! @returns
  //!   Master secret or alert number.
  string|int server_derive_master_secret(string data, string server_random, string client_random, array(int) version)
  {
    string premaster_secret;

    SSL3_DEBUG_MSG("server_derive_master_secret: ke_method %d\n",
		   session->ke_method);
    switch(session->ke_method)
    {
    default:
      error( "Internal error\n" );
#if 0
      /* What is this for? */
    case 0:
      return 0;
#endif
    case KE_dhe_dss:
    case KE_dhe_rsa:
      if (!sizeof(data))
      {
	/* Implicit encoding; Should never happen unless we have
	 * requested and received a client certificate of type
	 * rsa_fixed_dh or dss_fixed_dh. Not supported. */
	SSL3_DEBUG_MSG("SSL.handshake: Client uses implicit encoding if its DH-value.\n"
		       "               Hanging up.\n");
	return ALERT_certificate_unknown;
      }
      /* Fall through */
    case KE_dh_anon:
      {
	SSL3_DEBUG_MSG("KE_DHE\n");
	anonymous = 1;

	/* Explicit encoding */
	ADT.struct struct = ADT.struct(data);

	if (catch
	  {
	    dh_state->set_other(struct->get_bignum());
	  } || !struct->is_empty())
	{
	  return ALERT_unexpected_message;
	}

	premaster_secret = dh_state->get_shared()->digits(256);
	dh_state = 0;
	break;
      }
    case KE_rsa:
      {
	SSL3_DEBUG_MSG("KE_RSA\n");
	/* Decrypt the premaster_secret */
	SSL3_DEBUG_MSG("encrypted premaster_secret: %O\n", data);
	SSL3_DEBUG_MSG("temp_key: %O\n"
		       "session->rsa: %O\n", temp_key, session->rsa);
	if(version[1] >= PROTOCOL_TLS_1_0) {
	  if(sizeof(data)-2 == data[0]*256+data[1]) {
	    premaster_secret = (temp_key || session->rsa)->decrypt(data[2..]);
	  }
	} else {
	  premaster_secret = (temp_key || session->rsa)->decrypt(data);
	}
	SSL3_DEBUG_MSG("premaster_secret: %O\n", premaster_secret);
	if (!premaster_secret
	    || (sizeof(premaster_secret) != 48)
	    || (premaster_secret[0] != 3)
	    || (premaster_secret[1] != client_version[1]))
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
		   client_version[0], client_version[1]);
	  }
#endif

	  premaster_secret = context->random(48);
	  message_was_bad = 1;

	} else {
	}
	break;
      }
    }
    string res = "";

    .Cipher.MACsha sha = .Cipher.MACsha();
    .Cipher.MACmd5 md5 = .Cipher.MACmd5();

    if(version[1] == PROTOCOL_SSL_3_0) {
      foreach( ({ "A", "BB", "CCC" }), string cookie)
	res += md5->hash_raw(premaster_secret
			     + sha->hash_raw(cookie + premaster_secret 
					     + client_random + server_random));
    }
    else if(version[1] >= PROTOCOL_TLS_1_0) {
      res=.Cipher.prf(premaster_secret,"master secret",
		      client_random+server_random,48);
    }

    SSL3_DEBUG_MSG("master: %s\n", String.string2hex(res));
    return res;
  }

  int server_key_exchange(ADT.struct input, string server_random, string client_random)
  {
    SSL3_DEBUG_MSG("SSL.session: SERVER_KEY_EXCHANGE\n");

    ADT.struct temp_struct = ADT.struct();
    switch(session->ke_method) {
    case KE_rsa:
      SSL3_DEBUG_MSG("KE_RSA\n");
      Gmp.mpz n = input->get_bignum();
      Gmp.mpz e = input->get_bignum();
      temp_struct->put_bignum(n);
      temp_struct->put_bignum(e);
      Crypto.RSA rsa = Crypto.RSA();
      rsa->set_public_key(n, e);
      context->long_rsa = session->rsa;
      context->short_rsa = rsa;
      if (session->cipher_spec->is_exportable) {
	temp_key = rsa;
      }
      break;
    case KE_dhe_dss:
    case KE_dhe_rsa:
    case KE_dh_anon:
      SSL3_DEBUG_MSG("KE_DHE\n");
      Gmp.mpz p = input->get_bignum();
      Gmp.mpz g = input->get_bignum();
      Gmp.mpz order = [object(Gmp.mpz)]((p-1)/2); // FIXME: Is this correct?
      temp_struct->put_bignum(p);
      temp_struct->put_bignum(g);
      context->dh_ke =
	.Cipher.DHKeyExchange(.Cipher.DHParameters(p, g, order));
      context->dh_ke->set_other(input->get_bignum());
      temp_struct->put_bignum(context->dh_ke->other);
      break;
    }
    Gmp.mpz signature = input->get_bignum();
    int verification_ok;
    if( catch{ verification_ok = session->cipher_spec->verify(
          session, client_random + server_random, temp_struct, signature); }
      || !verification_ok)
    {
      return -1;
    }
    return 0;
  }

}

#if 0
class mac_none
{
  /* Dummy MAC algorithm */
  string hash(string data, Gmp.mpz seq_num) { return ""; }
}
#endif

//! MAC using SHA.
//!
//! @note
//!   Note: This uses the algorithm from the SSL 3.0 draft.
class MACsha
{
  inherit MACAlgorithm;

  protected constant pad_1 =  "6666666666666666666666666666666666666666";
  protected constant pad_2 = ("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\"
			   "\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\");

  protected Crypto.Hash algorithm = Crypto.SHA1;
  protected string secret;

  //! The length of the header prefixed by @[hash()].
  constant hash_header_size = 11;

  string hash_raw(string data)
  {
    return algorithm->hash(data);
  }

  //!
  string hash(object packet, Gmp.mpz seq_num)
  {
    string s = sprintf("%~8s%c%2c%s",
		       "\0\0\0\0\0\0\0\0", seq_num->digits(256),
		       packet->content_type, sizeof(packet->fragment),
		       packet->fragment);
    return hash_raw(secret + pad_2 +
		    hash_raw(secret + pad_1 + s));
  }

  //!
  string hash_master(string data, string|void s)
  {
    s = s || secret;
    return hash_raw(s + pad_2 +
		hash_raw(data + s + pad_1));
  }

  //!
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

  protected constant pad_1 =  "666666666666666666666666666666666666666666666666";
  protected constant pad_2 = ("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\"
			   "\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\");
  
  protected Crypto.Hash algorithm = Crypto.MD5;
}

//! HMAC using SHA.
//!
//! This is the MAC algorithm used by TLS 1.0 and later.
class MAChmac_sha {
  inherit MACAlgorithm;

  protected string secret;
  protected Crypto.HMAC hmac;

  //! The length of the header prefixed by @[hash()].
  constant hash_header_size = 13;

  string hash_raw(string data)
  {
    return hmac(secret)(data);
  }

  //!
  string hash(object packet, Gmp.mpz seq_num) {

    string s = sprintf("%~8s%c%c%c%2c%s",
		       "\0\0\0\0\0\0\0\0", seq_num->digits(256),
		       packet->content_type,
		       packet->protocol_version[0],packet->protocol_version[1],
		       sizeof(packet->fragment),
		       packet->fragment);

    return hmac(secret)(s);
  }

  //!
  protected void create(string|void s) {
    secret = s || "";
    hmac=Crypto.HMAC(Crypto.SHA1);
  }
}

//! HMAC using MD5.
//!
//! This is the MAC algorithm used by TLS 1.0 and later.
class MAChmac_md5 {
  inherit MAChmac_sha;

  //!
  protected void create(string|void s) {
    secret = s || "";
    hmac=Crypto.HMAC(Crypto.MD5);
  }
}

// Hashfn is either a Crypto.MD5 or Crypto.SHA
protected string P_hash(Crypto.Hash hashfn, int hlen, string secret,
		     string seed, int len) {
   
  Crypto.HMAC hmac=Crypto.HMAC(hashfn);
  string temp=seed;
  string res="";
  
  int noblocks=(int)ceil((1.0*len)/hlen);

  for(int i=0 ; i<noblocks ; i++) {
    temp=hmac(secret)(temp);
    res+=hmac(secret)(temp+seed);
  }
  return res[..(len-1)];
} 

//! The Pseudo Random Function used to derive the secret keys.
string prf(string secret,string label,string seed,int len) { 

  string s1=secret[..(int)(ceil(sizeof(secret)/2.0)-1)];
  string s2=secret[(int)(floor(sizeof(secret)/2.0))..];

  string a=P_hash(Crypto.MD5,16,s1,label+seed,len);
  string b=P_hash(Crypto.SHA1,20,s2,label+seed,len);

  return a ^ b;
}

//!
class DES
{
  inherit Crypto.CBC;

  protected void create() { ::create(Crypto.DES()); }

  this_program set_encrypt_key(string k)
  {
    ::set_encrypt_key(Crypto.DES->fix_parity(k));
    return this;
  }

  this_program set_decrypt_key(string k)
  {
    ::set_decrypt_key(Crypto.DES->fix_parity(k));
    return this;
  }
}

//!
class DES3
{
  inherit Crypto.CBC;

  protected void create() {
    ::create(Crypto.DES3());
  }

  this_program set_encrypt_key(string k)
  {
    ::set_encrypt_key(Crypto.DES3->fix_parity(k));
    return this;
  }

  this_program set_decrypt_key(string k)
  {
    ::set_decrypt_key(Crypto.DES3->fix_parity(k));
    return this;
  }
}

//!
class IDEA
{
  inherit Crypto.CBC;
  protected void create() { ::create(Crypto.IDEA()); }
}

//!
class AES
{
  inherit Crypto.CBC;
  protected void create() { ::create(Crypto.AES()); }
}

#if constant(Crypto.CAMELLIA)
//!
class CAMELLIA
{
  inherit Crypto.CBC;
  protected void create() { ::create(Crypto.CAMELLIA()); }
}
#endif

//! Signing using RSA.
ADT.struct rsa_sign(object context, string cookie, ADT.struct struct)
{
  /* Exactly how is the signature process defined? */
  
  string params = cookie + struct->contents();
  string digest = Crypto.MD5->hash(params) + Crypto.SHA1->hash(params);
      
  object s = context->rsa->raw_sign(digest);

  struct->put_bignum(s);
  return struct;
}

//! Verify an RSA signature.
int(0..1) rsa_verify(object context, string cookie, ADT.struct struct,
	       Gmp.mpz signature)
{
  /* Exactly how is the signature process defined? */

  string params = cookie + struct->contents();
  string digest = Crypto.MD5->hash(params) + Crypto.SHA1->hash(params);

  return context->rsa->raw_verify(digest, signature);
}

//! Signing using DSA.
ADT.struct dsa_sign(object context, string cookie, ADT.struct struct)
{
  /* NOTE: The details are not described in the SSL 3 spec. */
  string s = context->dsa->pkcs_sign(cookie + struct->contents(), Crypto.SHA1);
  struct->put_var_string(s, 2); 
  return struct;
}

//! The NULL signing method.
ADT.struct anon_sign(object context, string cookie, ADT.struct struct)
{
  return struct;
}

//! Diffie-Hellman parameters.
class DHParameters
{
  Gmp.mpz p, g, order;
  //!

  /* Default prime and generator, taken from the ssh2 spec:
   *
   * "This group was taken from the ISAKMP/Oakley specification, and was
   *  originally generated by Richard Schroeppel at the University of Arizona.
   *  Properties of this prime are described in [Orm96].
   *...
   *  [Orm96] Orman, H., "The Oakley Key Determination Protocol", version 1,
   *  TR97-92, Department of Computer Science Technical Report, University of
   *  Arizona."
   */

  /* p = 2^1024 - 2^960 - 1 + 2^64 * floor( 2^894 Pi + 129093 ) */

  protected Gmp.mpz orm96() {
    p = Gmp.mpz("FFFFFFFF FFFFFFFF C90FDAA2 2168C234 C4C6628B 80DC1CD1"
		"29024E08 8A67CC74 020BBEA6 3B139B22 514A0879 8E3404DD"
		"EF9519B3 CD3A431B 302B0A6D F25F1437 4FE1356D 6D51C245"
		"E485B576 625E7EC6 F44C42E9 A637ED6B 0BFF5CB6 F406B7ED"
		"EE386BFB 5A899FA5 AE9F2411 7C4B1FE6 49286651 ECE65381"
		"FFFFFFFF FFFFFFFF", 16);
    order = (p-1) / 2;

    g = Gmp.mpz(2);

    return this;
  }

  //!
  protected void create(object ... args) {
    switch (sizeof(args))
    {
    case 0:
      orm96();
      break;
    case 3:
      [p, g, order] = args;
      break;
    default:
      error( "Wrong number of arguments.\n" );
    }
  }
}

//! Implements Diffie-Hellman key-exchange.
//!
//! The following key exchange methods are implemented here:
//! @[KE_dhe_dss], @[KE_dhe_rsa] and @[KE_dh_anon].
class DHKeyExchange
{
  /* Public parameters */
  DHParameters parameters;

  Gmp.mpz our; /* Our value */
  Gmp.mpz other; /* Other party's value */
  Gmp.mpz secret; /* our =  g ^ secret mod p */

  //!
  protected void create(DHParameters p) {
    parameters = p;
  }

  this_program new_secret(function random)
  {
    secret = Gmp.mpz(random( (parameters->order->size() + 10 / 8)), 256)
      % (parameters->order - 1) + 1;

    our = parameters->g->powm(secret, parameters->p);
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
//!   Minor version of the SSL protocol to support.
//!
//! @returns
//!   Returns @expr{0@} (zero) for unsupported combinations.
//!   Otherwise returns an array with the following fields:
//!   @array
//!     @elem KeyExchangeType 0
//!       Key exchange method.
//!     @elem CipherSpec 1
//!       Initialized @[CipherSpec] for the @[suite].
//!   @endarray
array lookup(int suite, ProtocolVersion|int version)
{
  CipherSpec res = CipherSpec();
  int ke_method;
  
  array algorithms = CIPHER_SUITES[suite];
  if (!algorithms)
    return 0;

  ke_method = algorithms[0];

  switch(ke_method)
  {
  case KE_rsa:
  case KE_dhe_rsa:
    res->sign = rsa_sign;
    res->verify = rsa_verify;
    break;
  case KE_dhe_dss:
    res->sign = dsa_sign;
    break;
  case KE_dh_anon:
    res->sign = anon_sign;
    break;
  default:
    error( "Internal error.\n" );
  }

  switch(algorithms[1])
  {
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
  case CIPHER_idea:
    res->bulk_cipher_algorithm = IDEA;
    res->cipher_type = CIPHER_block;
    res->is_exportable = 0;
    res->key_material = 16;
    res->iv_size = 8;
    res->key_bits = 128;
    break;
  case CIPHER_aes:
    res->bulk_cipher_algorithm = AES;
    res->cipher_type = CIPHER_block;
    res->is_exportable = 0;
    res->key_material = 16;
    res->iv_size = 16;
    res->key_bits = 128;
    break;
  case CIPHER_aes256:
    res->bulk_cipher_algorithm = AES;
    res->cipher_type = CIPHER_block;
    res->is_exportable = 0;
    res->key_material = 32;
    res->iv_size = 16;
    res->key_bits = 256;
    break;
#if constant(Crypto.CAMELLIA)
  case CIPHER_camellia128:
    res->bulk_cipher_algorithm = CAMELLIA;
    res->cipher_type = CIPHER_block;
    res->is_exportable = 0;
    res->key_material = 16;
    res->iv_size = 16;
    res->key_bits = 128;
    break;
  case CIPHER_camellia256:
    res->bulk_cipher_algorithm = CAMELLIA;
    res->cipher_type = CIPHER_block;
    res->is_exportable = 0;
    res->key_material = 32;
    res->iv_size = 16;
    res->key_bits = 256;
    break;
#endif
  default:
    return 0;
  }

  switch(algorithms[2])
  {
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

  return ({ ke_method, res });
}

#else // constant(Crypto.Hash)
constant this_program_does_not_exist = 1;
#endif
