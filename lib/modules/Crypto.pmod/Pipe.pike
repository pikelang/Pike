
static array ciphers = ({});
static int _block_size = 1;
static int(0..1) reversed;

static void create(program|object|array(program|mixed) ... c) {
  if(!sizeof(c)) error("Too few arguments.\n");
  foreach(c, program|object|array cc) {
    if(objectp(cc))
      ciphers += ({ cc });
    else if(programp(cc))
      ciphers += ({ cc() });
    else {
      if(!sizeof(cc)) error("Empty array as argument.\n");
      ciphers += ({ cc[0](@cc[1..]) });
    }
  }

  foreach(ciphers, object c) {
    array a = Math.factor(_block_size);
    array b = Math.factor(c->block_size());
    array common = a&b;

    foreach(common, int x) {
      int pos = search(a,x);
      if(pos!=-1)
	a[pos]=1;
      pos = search(b,x);
      if(pos!=-1)
	b[pos]=1;
    }

    _block_size = `*( @(common+a+b) );
  }

}

string name() {
  return "pipe(" + ciphers->name()*", "+ ")";
}

int block_size() {
  return _block_size;
}

array(int) key_size() {
  return ciphers->key_size();
}

this_program set_encrypt_key(array(string)|string ... keys) {
  if(arrayp(keys[0]))
    keys = keys[0];
  if(sizeof(keys)!=sizeof(ciphers))
    error("Wrong number of keys.\n");
  if(reversed) {
    ciphers = reverse(ciphers);
    reversed = 0;
  }
  foreach(ciphers; int num; object c)
    c->set_encrypt_key( keys[num] );
  return this;
}

this_program set_decrypt_key(array(string)|string ... keys) {
  if(arrayp(keys[0]))
    keys = keys[0];
  if(sizeof(keys)!=sizeof(ciphers))
    error("Wrong number of keys.\n");
  if(!reversed) {
    ciphers = reverse(ciphers);
    reversed = 1;
  }
  keys = reverse(keys);
  foreach(ciphers; int num; object c)
    c->set_decrypt_key( keys[num] );
  return this;
}

string crypt(string data) {
  if(sizeof(data)%_block_size)
    error("Data size not integral number of blocks.\n");
  foreach(ciphers, object c)
    data = c->crypt(data);
  return data;
}
