/* cipher.pike
 *
 */

inherit "constants";

class CipherSpec {
  program bulk_cipher_algorithm;
  int cipher_type;
  program mac_algorithm;
  int is_exportable;
  int hash_size;
  int key_material;
  int iv_size;
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
  string pad_1 =  "6666666666666666666666666666666666666666";
  string pad_2 = ("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\"
		  "\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\");

  program algorithm = Crypto.sha;

  string secret;

  string hash_raw(string data)
  {
    object h = algorithm();
    h->update(data);
    return h->digest();
  }
  
  string hash(object packet, object seq_num)
  {
    string s = sprintf("%~8s%c%2c%s",
		       "\0\0\0\0\0\0\0\0", seq_num->digits(256),
		       packet->content_type, strlen(packet->fragment),
		       packet->fragment);
#ifdef SSL3_DEBUG
//    werror(sprintf("SSL.cipher: hashing '%s'\n", s));
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

  string pad_1 =  "666666666666666666666666666666666666666666666666";
  string pad_2 = ("\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\"
		  "\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\");
  
  program algorithm = Crypto.md5;
}

#if 0
class crypt_none
{
  /* Dummy stream cipher */
  object set_encrypt_key(string k) { return this_object(); }
  object set_decrypt_key(string k) { return this_object(); }  
  string crypt(string s) { return s; }
}
#endif

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

class rsa_auth
{
}

/* Return array of auth_method, cipher_spec */
array lookup(int suite)
{
  object res = CipherSpec();
  int ke_method;
  
  array algorithms = CIPHER_SUITES[suite];
  if (!algorithms)
    return 0;

  ke_method = algorithms[0];

  switch(algorithms[1])
  {
  case CIPHER_rc4:
    res->bulk_cipher_algorithm = Crypto.rc4;
    res->cipher_type = CIPHER_stream;
    res->is_exportable = 0;
    res->key_material = 16;
    res->iv_size = 0;
    break;
  case CIPHER_rc4_40:
    res->bulk_cipher_algorithm = Crypto.rc4;
    res->cipher_type = CIPHER_stream;
    res->is_exportable = 1;
    res->key_material = 16;
    res->iv_size = 0;
    break;
  case CIPHER_des:
    res->bulk_cipher_algorithm = des;
    res->cipher_type = CIPHER_block;
    res->is_exportable = 0;
    res->key_material = 7;
    res->iv_size = 8;
    break;
  case CIPHER_3des:
    res->bulk_cipher_algorithm = des3;
    res->cipher_type = CIPHER_block;
    res->is_exportable = 0;
    res->key_material = 16;
    res->iv_size = 8;
    break;
  case CIPHER_idea:
    res->bulk_cipher_algorithm = Crypto.idea_cbc;
    res->cipher_type = CIPHER_block;
    res->is_exportable = 0;
    res->key_material = 16;
    res->iv_size = 8;
    break;
  case CIPHER_null:
    res->bulk_cipher_algorithm = 0;
    res->cipher_type = CIPHER_stream;
    res->is_exportable = 1;
    res->key_material = 0;
    res->iv_size = 0;
    break;
  default:
    return 0;
  }

  switch(algorithms[2])
  {
  case HASH_sha:
    res->mac_algorithm = mac_sha;
    res->hash_size = 20;
    break;
  case HASH_md5:
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

  
