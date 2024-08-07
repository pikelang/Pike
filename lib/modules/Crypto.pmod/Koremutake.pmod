
#pike __REAL_VERSION__
#pragma strict_types

//! Quote from Koremutake home page @url{http://shorl.com/koremutake@}:
//!
//! In an attempt to temporarily solve the fact that human beings seem
//! to be inable to remember important things (such as their names, car
//! keys and seemingly random numbers with fourteen digits in 'em), we
//! invented Koremutake.
//!
//! It is, in plain language, a way to express any large number as a
//! sequence of syllables. The general idea is that word-sounding
//! pieces of information are a lot easier to remember than a sequence
//! of digits.
//!
//! @note
//!   Note this encoding is NOT suitable for encoding numeric
//!   quantities where initial zeroes are significant (ie it
//!   is NOT suitable for eg PIN-numbers).
//!
//! @note
//!   This module implements an API that is similar to (but NOT compatible
//!   with) the generic @[Crypto.Cipher] API.

protected local constant array(string(7bit)) table = ({
 "BA", "BE", "BI", "BO", "BU", "BY", "DA", "DE",
 "DI", "DO", "DU", "DY", "FA", "FE", "FI", "FO",
 "FU", "FY", "GA", "GE", "GI", "GO", "GU", "GY",
 "HA", "HE", "HI", "HO", "HU", "HY", "JA", "JE",
 "JI", "JO", "JU", "JY", "KA", "KE", "KI", "KO",
 "KU", "KY", "LA", "LE", "LI", "LO", "LU", "LY",
 "MA", "ME", "MI", "MO", "MU", "MY", "NA", "NE",
 "NI", "NO", "NU", "NY", "PA", "PE", "PI", "PO",
 "PU", "PY", "RA", "RE", "RI", "RO", "RU", "RY",
 "SA", "SE", "SI", "SO", "SU", "SY", "TA", "TE",
 "TI", "TO", "TU", "TY", "VA", "VE", "VI", "VO",
 "VU", "VY", "BRA", "BRE", "BRI", "BRO", "BRU", "BRY",
 "DRA", "DRE", "DRI", "DRO", "DRU", "DRY", "FRA", "FRE",
 "FRI", "FRO", "FRU", "FRY", "GRA", "GRE", "GRI", "GRO",
 "GRU", "GRY", "PRA", "PRE", "PRI", "PRO", "PRU", "PRY",
 "STA", "STE", "STI", "STO", "STU", "STY", "TRA", "TRE"
});

protected local constant mapping(string(7bit):int(7bit)) lookup =
  mkmapping(table, indices(table));

//! Encode an integer as a koremutake string.
//!
//! @note
//!   Encrypts an integer. This is not compatible with the
//!   @[Crypto.Cipher] API.
string(7bit) encrypt(int(0..) m) {
  string(7bit) c="";
  while(m) {
    c = table[m&127] + c;
    m >>= 7;
  }
  return c;
}

//! Decode a koremutake string into an integer.
//!
//! @note
//!   Returns an integer. This is not compatible with the
//!   @[Crypto.Cipher] API.
int(0..) decrypt(string(7bit) c) {
  int(0..) m;
  c = [string(7bit)]upper_case(c);
  while(sizeof(c)) {
    if(sizeof(c)==1) error("Error in cryptogram.\n");
    int(0..1) trigramp = (< 'R', 'T' >)[c[1]];
    string(7bit) w = c[..1 + trigramp];
    c = c[2+trigramp..];
    int(7bit) p = lookup[w];
    if (undefinedp(p)) error("Error in cryptogram %O.\n", w);
    m <<= 7;
    m += p;
  }
  return m;
}


// Cipher interface

string(7bit) name() { return "koremutake"; }
int block_size() { return 1; }
int key_size() { return 0; }

//!
class `() {

  string(7bit) name() { return "koremutake"; }
  int block_size() { return 1; }
  int key_size() { return 0; }

  protected int mode;
  this_program set_encrypt_key(void|mixed key) {
    key;
    mode = 0;
    return this;
  }
  this_program set_decrypt_key(void|mixed key) {
    key;
    mode = 1;
    return this;
  }
  this_program make_key() { return this; }

  //! @note
  //!   Encrypts or returns an integer. This is not compatible with the
  //!   @[Crypto.Cipher] API.
  int(0..)|string(7bit) crypt(int(0..)|string(7bit) x) {
    if(mode) {
      if(!stringp(x)) error("Wrong type. Expected string.\n");
      return decrypt([string]x);
    }
    else {
      if(!intp(x)) error("Wrong type. Expected int.\n");
      return encrypt([int]x);
    }
  }
}
