
//! Implements a simple substitution crypto, ie. one of the first crypto
//! systems ever invented and thus one of the least secure ones available.

static mapping(string:string|array(string)) enc_key = ([]);
static mapping(string:string) dec_key = ([]);
static int(0..1) is_expandable;
static array(string) null_chars = ({});
static int null_fq;

static constant AZ = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"/1;
static constant az = "abcdefghijklmnopqrstuvwxyz"/1;

static array(int) charify(array(string) x) {
  return map(x, lambda(string y) { return y[0]; });
}

//! Sets the encryption and decryption key. The decryption key is
//! derived from the encryption @[key] by reversing the mapping. If
//! one index maps to an array of strings, one element from the array
//! will be chosen at random in such substitution.
//! @throws
//!   An error is thrown if the encryption key can not be made reversible.
this_program set_key(mapping(string:string|array(string)) key) {
  enc_key = key;

  is_expandable = 0;
  foreach(key; string from; mixed to)
    if(arrayp(to)) {
      is_expandable = 1;
    }
    else if(from==to)
      m_delete(key, from);

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
  return this_object();
}

//! Set null characters (fillers). Characters from @[chars] will be
//! inserted into the output stream with a probability @[p].
//! @param p
//!   A float between 0.0 and 1.0 or an integer between 0 and 100.
//! @param chars
//!   An array of one character strings.
this_program set_null_chars(int|float p, array chars) {
  if(floatp(p)) p = (int)(100*p);
  if(p<0) error("Probability must not be negative.\n");
  null_fq = p;
  null_chars = chars;
  foreach(null_chars, string char) {
    if( !(< 0, "" >)[dec_key[char]] )
      error("Null character part of key.\n");
    dec_key[char]="";
  }
  return this_object();
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
this_program set_rot_key(void|int steps, void|array alphabet) {
  if(!steps) steps=13;
  if(alphabet)
    set_key(make_rot_map(steps, alphabet));
  else
    set_key(make_rot_map(steps, az)+make_rot_map(steps, AZ));
  return this_object();
}

static string reduce_word(string|array(int) w, array(string) alpha) {
  w = (array)w;
  w = Array.uniq(w);
  multiset a = (multiset)charify(alpha);
  foreach(w;; int char)
    if(!a[char]) error("Passphrase character %c not in alphabet.\n", char);
  return (string)w;
}

static array(string) scramble_alpha(string pwd, array(string) alpha, int off) {
  array out = pwd/1;
  alpha -= out;
  out += alpha;
  off %= sizeof(alpha);
  if(off) return out[off..] + out[0..off-1];
  return out;
}

//! Sets the key according to ACA K1 key generation. The plaintext
//! alphabet is prepended with a keyword @[key] that shifts the alphabet
//! positions compared to the cryptogram alphabet. The plaintext
//! alphabet is then reduced with the characters in the keyword. It is
//! also optionally rotated @[offset] number of steps.
this_program set_ACA_K1_key(string key, void|int offset, void|array alphabet) {
  if(!alphabet) alphabet = AZ;
  key = reduce_word(key, alphabet);
  set_key( mkmapping(scramble_alpha(key, alphabet, offset), alphabet) );
  return this_object();
}

//! Sets the key according to ACA K2 key generation. The cryptogram
//! alphabet is prepended with a keyword @[key] that shifts the alphabet
//! positions compared to the plaintext alphabet. The cryptogram
//! alphabet is then reduced with the characters in the keyword. It is
//! als optionally reotated @[offset] number of steps.
this_program set_ACA_K2_key(string key, void|int offset, void|array alphabet) {
  if(!alphabet) alphabet = AZ;
  key = reduce_word(key, alphabet);
  set_key( mkmapping(alphabet, scramble_alpha(key, alphabet, offset)) );
  return this_object();
}

//! Sets the key according to ACA K3 key generation. Both the plaintext
//! and the cryptogram alphabets are prepended with a keyword @[key],
//! which characters are removed from the rest of the alphabet. The
//! plaintext alphabet is then rotated @[offset] number of steps.
this_program set_ACA_K3_key(string key, int offset, void|array alphabet) {
  if(!offset) error("Dummy key! Offset must be != 0.\n");
  if(!alphabet) alphabet = AZ;
  key = reduce_word(key, alphabet);
  set_key( mkmapping(scramble_alpha(key, alphabet, offset),
		     scramble_alpha(key, alphabet, 0)) );
  return this_object();
}

//! Sets the key according to ACA K4 key generation. Both the plaintext
//! and the cryptogram alphabets are prepended with the keywords @[key1]
//! and @[key2]. The plaintext alphabet is then rotated @[offset] number
//! of steps.
this_program set_ACA_K4_key(string key1, string key2,
			    void|int offset, void|array alphabet) {
  if(!alphabet) alphabet = AZ;
  key1 = reduce_word(key1, alphabet);
  key2 = reduce_word(key2, alphabet);
  set_key( mkmapping(scramble_alpha(key1, alphabet, offset),
		     scramble_alpha(key2, alphabet, 0)) );
  return this_object();
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

//! Removes characters not in the encryption key or in
//! the @[save] multiset from the message @[m].
string filter(string m, void|multiset(int) save) {
  if(!save) save=(<>);
  save += (multiset)charify(indices(enc_key));
  m = predef::filter(m, lambda(int c) { return save[c]; });
  return m;
}
