#pike 7.5

//! @deprecated Crypto.CBC

#if constant(Nettle.CBC)
inherit Nettle.CBC;

static class Wrapper(object a) {
  int block_size() { return a->query_block_size(); }
  int key_size() { return a->query_key_length(); }
  function(string:void) set_encrypt_key = a->set_encrypt_key;
  function(string:void) set_decrypt_key = a->set_decrypt_key;
  string crypt(string data) { return a->crypt_block(data); }
}

static void create(program|object a) {
  if(a->crypt)
    ::create(a);
  else
    ::create(Wrapper(a));
}

int query_key_length() { return key_size(); }
int query_block_size() { return block_size(); }

static int(0..1) mode;
this_program set_encrypt_key(string key) {
  mode = 0;
  return ::set_encrypt_key(key);
}

this_program set_decrypt_key(string key) {
  mode = 1;
  return ::set_decrypt_key(key);
}

string encrypt_block(string data) {
  if(!mode)
    return crypt(data);
  else
    error("CBC is in decrypt mode.\n");
}

string decrypt_block(string data) {
  if(mode)
    return crypt(data);
  else
    error("CBC is in encrypt mode.\n");
}

string crypt_block(string p) { return crypt(p); }

#endif
