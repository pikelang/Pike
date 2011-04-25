#pike 7.4

/* $Id$
 *
 */

#pragma dont_save_parent
// Override if someone set __pragma_save_parent__ globally. Other code
// assumes the constants can be indexed out of this class directly. It
// should probably be a pmod instead. /mast

//! Encryption and MAC algorithms used in SSL.
//! @deprecated SSL.Cipher

inherit "constants";

class CipherSpec {
  program bulk_cipher_algorithm;
  int cipher_type;
  program mac_algorithm;
  int is_exportable;
  int hash_size;
  int key_material;
  int iv_size;
  int key_bits;
  function sign;
  function verify;
}

#if 0
class mac_none
{
  /* Dummy MAC algorithm */
//  string hash_raw(string data) { return ""; }
  string hash(string data, object seq_num) { return ""; }
}
#endif

class mac_sha
{
  constant pad_1 =  "6666666666666666666666666666666666666666";
  constant pad_2 = ("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\"
		    "\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\");

  constant algorithm = Crypto.sha;

  string secret;

  string hash_raw(string data)
  {
#ifdef SSL3_DEBUG_CRYPT
    werror(sprintf("SSL.cipher: hash_raw(%O)\n", data));
#endif
    
    object h = algorithm();
    string res = h->update(data)->digest();
#ifdef SSL3_DEBUG_CRYPT
    werror(sprintf("SSL.cipher: hash_raw->%O\n",res));
#endif
    
    return res;
  }
  
  string hash(object packet, object seq_num)
  {
    string s = sprintf("%~8s%c%2c%s",
		       "\0\0\0\0\0\0\0\0", seq_num->digits(256),
		       packet->content_type, strlen(packet->fragment),
		       packet->fragment);
#ifdef SSL3_DEBUG_CRYPT
//    werror(sprintf("SSL.cipher: hashing %O\n", s));
#endif
    return hash_raw(secret + pad_2 +
		    hash_raw(secret + pad_1 + s));
  }

  string hash_master(string data, string|void s)
  {
    s = s || secret;
    return hash_raw(s + pad_2 +
		hash_raw(data + s + pad_1));
  }

  void create (string|void s)
  {
    secret = s || "";
  }
}

class mac_md5 {
  inherit mac_sha;

  constant pad_1 =  "666666666666666666666666666666666666666666666666";
  constant pad_2 = ("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\"
		    "\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\");
  
  constant algorithm = Crypto.md5;
}

class mac_hmac_sha {

  string secret;
  object hmac;
  
  string hash(object packet,object seq_num) {

    string s = sprintf("%~8s%c%c%c%2c%s",
		       "\0\0\0\0\0\0\0\0", seq_num->digits(256),
		       packet->content_type,
		       packet->protocol_version[0],packet->protocol_version[1],
		       strlen(packet->fragment),
		       packet->fragment);

    return  hmac(secret)(s);
  }

  void create(string|void s) {
    secret = s || "";
    hmac=Crypto.hmac(Crypto.sha);
  }
}

class mac_hmac_md5 {
  inherit mac_hmac_sha;

  void create(string|void s) {
    secret = s || "";
    hmac=Crypto.hmac(Crypto.md5);
  }
}

// Hashfn is either a Crypto.md5 or Crypto.sha 
static string P_hash(object hashfn,int hlen,string secret,string seed,int len) {
   
  Crypto.hmac hmac=Crypto.hmac(hashfn);
  string temp=seed;
  string res="";
  
  int noblocks=(int)ceil((1.0*len)/hlen);

  for(int i=0 ; i<noblocks ; i++) {
    temp=hmac(secret)(temp);
    res+=hmac(secret)(temp+seed);
  }
    return res[..(len-1)];  
} 

string prf(string secret,string label,string seed,int len) { 

  string s1=secret[..(int)(ceil(strlen(secret)/2.0)-1)];
  string s2=secret[(int)(floor(strlen(secret)/2.0))..];

  string a=P_hash(Crypto.md5,16,s1,label+seed,len);
  string b=P_hash(Crypto.sha,20,s2,label+seed,len);

  return a ^ b;
}


class des
{
  inherit Crypto.des_cbc : c;

  object set_encrypt_key(string k)
  {
    c::set_encrypt_key(Crypto.des_parity(k));
    return this_object();
  }

  object set_decrypt_key(string k)
  {
    c::set_decrypt_key(Crypto.des_parity(k));
    return this_object();
  }
}

class des3
{
  inherit Crypto.des3_cbc : c;

  object set_encrypt_key(string k)
  {
    c::set_encrypt_key(Crypto.des_parity(k));
    return this_object();
  }

  object set_decrypt_key(string k)
  {
    c::set_decrypt_key(Crypto.des_parity(k));
    return this_object();
  }
}

object rsa_sign(object context, string cookie, object struct)
{
  /* Exactly how is the signature process defined? */
  
  string params = cookie + struct->contents();
  string digest = Crypto.md5()->update(params)->digest()
    + Crypto.sha()->update(params)->digest();    
      
  object s = context->rsa->raw_sign(digest);
#ifdef SSL3_DEBUG_CRYPT
  werror(sprintf("  Digest: '%O'\n"
		 "  Signature: '%O'\n",
		 digest, s->digits(256)));
#endif
  
  struct->put_bignum(s);
  return struct;
}

int rsa_verify(object context, string cookie, object struct,
	       object(Gmp.mpz) signature)
{
  /* Exactly how is the signature process defined? */

  string params = cookie + struct->contents();
  string digest = Crypto.md5()->update(params)->digest()
    + Crypto.sha()->update(params)->digest();

  return context->rsa->raw_verify(digest, signature);
}

object dsa_sign(object context, string cookie, object struct)
{
  /* NOTE: The details are not described in the SSL 3 spec. */
  string s = context->dsa->sign_ssl(cookie + struct->contents());
  struct->put_var_string(s, 2); 
  return struct;
}

object anon_sign(object context, string cookie, object struct)
{
  return struct;
}

class dh_parameters
{
  object p, g, order;

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
  
  object orm96()
    {
      p = Gmp.mpz("FFFFFFFF FFFFFFFF C90FDAA2 2168C234 C4C6628B 80DC1CD1"
		  "29024E08 8A67CC74 020BBEA6 3B139B22 514A0879 8E3404DD"
		  "EF9519B3 CD3A431B 302B0A6D F25F1437 4FE1356D 6D51C245"
		  "E485B576 625E7EC6 F44C42E9 A637ED6B 0BFF5CB6 F406B7ED"
		  "EE386BFB 5A899FA5 AE9F2411 7C4B1FE6 49286651 ECE65381"
		  "FFFFFFFF FFFFFFFF", 16);
      order = (p-1) / 2;
      
      g = Gmp.mpz(2);

      return this_object();
    }

  void create(object ... args)
    {
      switch (sizeof(args))
      {
      case 0:
	orm96();
	break;
      case 3:
#if 0
	[p, g, order] = args;
#else
	p = args[0]; g = args[1]; order = args[2];
#endif
	break;
      default:
	error( "SSL.cipher.dh_parameters->create: "
	       "Wrong number of arguments.\n" );
      }
    }
}

class dh_key_exchange
{
  /* Public parameters */
  object parameters;

  object our; /* Our value */
  object other; /* Other party's value */
  object secret; /* our =  g ^ secret mod p */

  void create(object p)
    {
      parameters = p;
    }

  object new_secret(function random)
    {
      secret = Gmp.mpz(random( (parameters->order->size() + 10 / 8)), 256)
	% (parameters->order - 1) + 1;

      our = parameters->g->powm(secret, parameters->p);
      return this_object();
    }
  
  object set_other(object o)
    {
      other = o;
      return this_object();
    }

  object get_shared()
    {
      return other->powm(secret, parameters->p);
    }
}

/* Return array of auth_method, cipher_spec */
array lookup(int suite,int version)
{
  object res = CipherSpec();
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
    error( "SSL.cipher.pike: Internal error.\n" );
  }

  switch(algorithms[1])
  {
  case CIPHER_rc4_40:
#if constant(Crypto.arcfour)
    res->bulk_cipher_algorithm = Crypto.arcfour;
#else /* !constant(Crypto.arcfour) */
    res->bulk_cipher_algorithm = Crypto.rc4;
#endif /* constant(Crypto.arcfour) */
    res->cipher_type = CIPHER_stream;
    res->is_exportable = 1;
    res->key_material = 16;
    res->iv_size = 0;
    res->key_bits = 40;
    break;
  case CIPHER_des40:
    res->bulk_cipher_algorithm = des;
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
#ifndef WEAK_CRYPTO_40BIT
  case CIPHER_rc4:
#if constant(Crypto.arcfour)
    res->bulk_cipher_algorithm = Crypto.arcfour;
#else /* !constant(Crypto.arcfour) */
    res->bulk_cipher_algorithm = Crypto.rc4;
#endif /* constant(Crypto.arcfour) */
    res->cipher_type = CIPHER_stream;
    res->is_exportable = 0;
    res->key_material = 16;
    res->iv_size = 0;
    res->key_bits = 128;
    break;
  case CIPHER_des:
    res->bulk_cipher_algorithm = des;
    res->cipher_type = CIPHER_block;
    res->is_exportable = 0;
    res->key_material = 8;
    res->iv_size = 8;
    res->key_bits = 56;
    break;
  case CIPHER_3des:
    res->bulk_cipher_algorithm = des3;
    res->cipher_type = CIPHER_block;
    res->is_exportable = 0;
    res->key_material = 24;
    res->iv_size = 8;
    res->key_bits = 168;
    break;
  case CIPHER_idea:
    res->bulk_cipher_algorithm = Crypto.idea_cbc;
    res->cipher_type = CIPHER_block;
    res->is_exportable = 0;
    res->key_material = 16;
    res->iv_size = 8;
    res->key_bits = 128;
    break;
#endif /* !WEAK_CRYPTO_40BIT (magic comment) */
  default:
    return 0;
  }

  switch(algorithms[2])
  {
  case HASH_sha:
    if(version==1)
      res->mac_algorithm = mac_hmac_sha;
    else
      res->mac_algorithm = mac_sha;
    res->hash_size = 20;
    break;
  case HASH_md5:
    if(version==1)
      res->mac_algorithm = mac_hmac_md5;
    else
      res->mac_algorithm = mac_md5;
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
