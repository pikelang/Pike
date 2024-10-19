
#pike __REAL_VERSION__
#pragma strict_types
#require constant(Crypto.CipherState)

//! A wrapper class that connects several cipher algorithms into one
//! algorithm. E.g. triple DES can be emulated with
//! @expr{Crypto.Pipe(Crypto.DES, Crypto.DES, Crypto.DES)@}.

protected array(.CipherState) ciphers = ({});
protected int _block_size = 1;
protected int(0..1) reversed;

protected int(0..1) is_crypto(object c) {
  return c->block_size && c->key_size && c->set_encrypt_key &&
    c->set_decrypt_key && c->crypt && 1;
}

protected void create(program|object|array(program|mixed) ... c) {
  if(!sizeof(c)) error("Too few arguments.\n");
  foreach(c, program|object|array cc) {
    if(objectp(cc)) {
      mixed mcc;
      if(!is_crypto([object]cc) && callablep(cc)) {
	mcc = cc();
	if(!objectp(mcc) && !is_crypto([object]mcc))
	  error("Not a crypto object.\n");
	cc = [object]mcc;
      }
      ciphers += ({ [object(.CipherState)]cc });
    }
    else if(programp(cc))
      ciphers += ({ ([program(.CipherState)]cc)() });
    else if(arrayp(cc)) {
      array acc = [array]cc;
      if(!sizeof(acc)) error("Empty array as argument.\n");
      if(!programp(acc[0])) error("First array element not program.\n");
      ciphers += ({ ([program(.CipherState)]acc[0])(@acc[1..]) });
    }
    else
      error("Wrong type in argument.\n");
  }

  foreach(ciphers, .CipherState c) {
    array(int) a = Math.factor(_block_size);
    array(int) b = Math.factor(c->block_size());
    array(int) common = a&b;

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
  return "Pipe(" + ciphers->name()*", "+ ")";
}

int block_size() {
  return _block_size;
}

array(int) key_size() {
  return ciphers->key_size();
}

this_program set_encrypt_key(array(string(8bit))|string(8bit) ... keys) {
  if(arrayp(keys[0]))
    keys = [array(string(8bit))]keys[0];
  if(sizeof(keys)!=sizeof(ciphers))
    error("Wrong number of keys.\n");
  if(reversed) {
    ciphers = reverse(ciphers);
    reversed = 0;
  }
  foreach(ciphers; int num; .CipherState c)
    c->set_encrypt_key( [string(8bit)]keys[num] );
  return this;
}

this_program set_decrypt_key(array(string(8bit))|string(8bit) ... keys) {
  if(arrayp(keys[0]))
    keys = [array(string(8bit))]keys[0];
  if(sizeof(keys)!=sizeof(ciphers))
    error("Wrong number of keys.\n");
  if(!reversed) {
    ciphers = reverse(ciphers);
    reversed = 1;
  }
  keys = reverse(keys);
  foreach(ciphers; int num; .CipherState c)
    c->set_decrypt_key( [string(8bit)]keys[num] );
  return this;
}

string(8bit) crypt(string(8bit) data) {
  if(sizeof(data)%_block_size)
    error("Data size not integral number of blocks.\n");
  foreach(ciphers, .CipherState c)
    data = c->crypt(data);
  return data;
}
