#pike 7.5

#if constant(Crypto.Pipe)
inherit Crypto.Pipe;

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

static void create(program|object|array(program|mixed) ... c) {
  array new_c = ({});
  foreach(c, program|object|array cc) {
    object new;
    if(objectp(cc))
      new = cc;
    else if(programp(cc))
      new = cc();
    else {
      if(!sizeof(cc)) error("Empty array as argument.\n");
      new = cc[0](@cc[1..]);
    }
    if( !new->block_size )
      new = Wrapper(new);
    new_c += ({ new });
  }
  ::create( @new_c );
}

string name() { return upper_case(::name()); }

array(int) query_key_length() { return key_size(); }
int query_block_size() { return block_size(); }
string crypt_block(string p) { return crypt(p); }

#else /* constant(Crypto.Pipe) */
constant this_program_does_not_exist=1;
#endif /* constant(Crypto.Pipe) */
