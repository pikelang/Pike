
//! Implements a simple substitution crypto, ie. one of the first crypto
//! systems ever invented and thus one of the least secure ones available.

static mapping(string:string|array(string)) enc_key = ([]);
static mapping(string:string) dec_key = ([]);
static int(0..1) is_expandable;
static array(string) null_chars = ({});
static int null_fq;

//! Sets the encryption and decryption key. The decryption key is
//! derived from the encryption @[key] by reversing the mapping. If
//! one index maps to an array of strings, one element from the array
//! will be chosen at random in such substitution.
//! @throws
//!   An error is thrown if the encryption key can not be made reversible.
void set_key(mapping(string:string|array(string)) key) {
  enc_key = key;

  is_expandable = 0;
  foreach(values(key), mixed to)
    if(arrayp(to)) {
      is_expandable = 1;
      break;
    }

  if(is_expandable) {
    dec_key = ([]);
    foreach(enc_key; string from; string|array(string) to) {
      if(stringp(to))
	dec_key[to] = from;
      else
	foreach(to, string to) {
	  if(dec_key[to] && dec_key[to]!=from)
	    error("Key not reversible.\n");
	  dec_key[to] = from;
	}
    }
  }
  else {
    dec_key = mkmapping(values(key),indices(key));
    if(sizeof(enc_key)!=sizeof(dec_key))
      error("Key not reversible.\n");
  }

  if(enc_key[""]) {
    array null = Array.uniq(null_chars +
			    Array.arrayify( m_delete(enc_key, "") ));
    if(null_fq) set_null_chars(null_fq, null);
  }
}

//! Set null characters (fillers). Characters from @[chars] will be
//! inserted into the output stream with a probability @[p].
//! @param p
//!   A float between 0.0 and 1.0 or an integer between 0 and 100.
//! @param chars
void set_null_chars(int|float p, array chars) {
  if(floatp(p)) p = (int)(100*p);
  if(p<0) error("Probability must not be negative.\n");
  null_fq = p;
  null_chars = chars;
  foreach(null_chars, string char) {
    if( !(< 0, "" >)[dec_key[char]] )
      error("Null character part of key.\n");
    dec_key[char]="";
  }
}

static mapping(string:string) make_rot_map(int steps, array alphabet) {
  mapping key = ([]);
  foreach(alphabet; int pos; string char)
    key[char] = alphabet[ (pos+steps)%sizeof(alphabet) ];
  return key;
}

//! Sets the key to a ROT substitution system. @[steps] defaults
//! to 13 and @[alphabet] defaults to A-Z, i.e. this function
//! defaults to set the substitution crypto to be ROT13. If no
//! alphabet is given the key will be case insensitive, e.g. the
//! key will really be two ROT13 alphabets, one a-z and one A-Z,
//! used simultaneously.
void set_rot_key(void|int steps, void|array alphabet) {
  if(!steps) steps=13;
  if(alphabet)
    set_key(make_rot_map(steps, alphabet));
  else
    set_key(make_rot_map(steps, "abcdefghijklmnopqrstuvwxyz"/1)+
	    make_rot_map(steps, "ABCDEFGHIJKLMNOPQRSTUVWXYZ"/1));
}

//! Encrypts the message @[m].
string encode(string m) {
  if(is_expandable || null_fq) {
    String.Buffer ret = String.Buffer(sizeof(m));
    foreach(m/1, string c) {
      string|array(string) to = enc_key[c];
      if(!to)
	ret->add(c);
      else if(stringp(to))
	ret->add(to);
      else
	ret->add(random(to));
      while(random(100)<null_fq)
	ret->add(random(null_chars));
    }
    return (string)ret;
  }
  return replace(m, enc_key);
}

//! Decrypts the cryptogram @[c].
string decode(string c) {
  return replace(c, dec_key);
}
