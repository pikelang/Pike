#pike 7.5

#if constant(Nettle.Proxy)
inherit Nettle.Proxy;

static class Wrapper(object a) {
  int _key_size;
  int block_size() {
    if(!a->query_block_size) return 1;
    return a->query_block_size() || 1;
  }
  int key_size() { return _key_size; }
  void set_encrypt_key(string key) {
    _key_size = sizeof(key);
    a->set_encrypt_key(key);
  }
  void set_decrypt_key(string key) {
    _key_size = sizeof(key);
    a->set_decrypt_key(key);
  }
  string crypt(string data) { return a->crypt_block(data); }
  string name() { return a->name(); }
}

static void create(program|object|array(program|mixed) c) {
  if(programp(c))
    c = c();
  else if(arrayp(c)){
    if(!sizeof(c)) error("Empty array as argument.\n");
    c = c[0](@c[1..]);
  }
  if( !c->block_size )
      c = Wrapper(c);
  ::create( @c );
}

string name() { return ::name(); }

int query_key_length() { return key_size(); }
int query_block_size() { return block_size(); }
string crypt_block(string p) { return crypt(p); }
string unpad(string m) {
  m = crypt(m);
  return ::unpad(m);
}

#endif
