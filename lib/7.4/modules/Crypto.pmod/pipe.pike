#pike 7.5

inherit Crypto.Pipe;

static class Wrapper(object a) {
  int block_size() { return a->query_block_size(); }
  int key_size() { return a->query_key_length(); }
  function(string:void) set_encrypt_key = a->set_encrypt_key;
  function(string:void) set_decrypt_key = a->set_decrypt_key;
  string crypt(string data) { return a->crypt_block(data); }
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
