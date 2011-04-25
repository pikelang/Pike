// $Id$

#pike __REAL_VERSION__

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

static constant table = ({
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

//! Encode an integer as a koremutake string.
string encrypt(int m) {
  string c="";
  while(m) {
    c = table[m&127] + c;
    m >>= 7;
  }
  return c;
}

//! Decode a koremutake string into an integer.
int decrypt(string c) {
  int m;
  c = upper_case(c);
  while(sizeof(c)) {
    if(sizeof(c)==1) error("Error in cryptogram.\n");
    string w = c[..1];
    c = c[2..];

    if( (< 'R', 'T' >)[w[1]] ) {
      if(!sizeof(c)) error("Error in cryptogram.\n");
      w += c[..0];
      c = c[1..];
    }

    int p = search(table, w);
    if(p==-1) error("Error in cryptogram %O.\n", w);
    m <<= 7;
    m += p;
  }
  return m;
}


// Cipher interface

string name() { return "koremutake"; }
int block_size() { return 1; }
int key_size() { return 0; }

class `() {

  string name() { return "koremutake"; }
  int block_size() { return 1; }
  int key_size() { return 0; }

  static int mode;
  this_program set_encrypt_key(void|mixed key) {
    mode = 0;
    return this;
  }
  this_program set_decrypt_key(void|mixed key) {
    mode = 1;
    return this;
  }
  this_program make_key() { return this; }

  int|string crypt(int|string x) {
    if(mode)
      return decrypt(x);
    else
      return encrypt(x);
  }
}
