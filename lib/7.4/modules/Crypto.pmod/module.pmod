#pike 7.6

//! From Pike 7.5 @[pw] and @[salt] are binary strings, so
//! the result is different if any of these includes @expr{"\0"@}.
//!
//! The return type is also changed from string to int|string. If
//! the second argument is a hash, @[pw] will be hashed with the
//! salt from the hash and compared with the hash.
string crypt_md5(string pw, void|string salt) {
  sscanf(pw, "%s\0", pw);
  sscanf(salt, "%s\0", salt);
  string|int res = Crypto.crypt_md5(pw,salt);
  if(res==1) return salt;
  if(stringp(res)) return res;
  sscanf(salt, "$1$%s$", salt);
  return Crypto.crypt_md5(pw,salt);
}

//! @deprecated String.string2hex
constant string_to_hex = __builtin.string2hex;

//! @deprecated String.hex2string
constant hex_to_string = __builtin.hex2string;

//! des_parity is removed. Instead there is @[Crypto.DES->fix_parity],
//! but it always outputs a key sized string.
string des_parity(string s) {
  foreach(s; int pos; int v) {
    int i = v & 0xfe;
    s[pos] = i | !Int.parity(i);
  }
  return s;
}

// These haven't been modified.
#if 0
constant des_cbc = Crypto.des_cbc;
constant des3_cbc = Crypto.des3_cbc;
constant dsa = Crypto.dsa;
constant hmac = Crypto.hmac;
constant idea_cbc = Crypto.idea_cbc;
constant randomness = Crypto.randomness;
constant rsa = Crypto.rsa;
#endif

