#pike __REAL_VERSION__

//! Returns the parity of the integer @[value]. If the
//! parity is odd 1 is returned. If it is even 0 is
//! returned.
int(0..1) parity(int(0..) value) {
  if(value<0) error("Parity can not determined for negative values.\n");
  return Gmp.mpz(value)->popcount()&1;
}

constant NATIVE_MIN = __builtin.NATIVE_INT_MIN;
constant NATIVE_MAX = __builtin.NATIVE_INT_MAX;

//! @decl constant NATIVE_MIN;
//! @decl constant NATIVE_MAX;
//!
//! The limits for using the native representation of integers on the
//! current architecture. Any integer that is outside this range uses
//! a more complex and slower representation. Also, some builtin
//! functions that don't expect very large integers might start to
//! complain about invalid argument type when given values outside
//! this range (they typically say something like "Expected integer,
//! got object").
//!
//! @[NATIVE_MIN] is not greater than @expr{-2147483648@}
//! (@expr{-0x80000000@}).
//!
//! @[NATIVE_MAX] is not less than @expr{2147483647@}
//! (@expr{0x7fffffff@}).
//!
//! @note
//! The size of the native integers can be controlled when Pike is
//! compiled with the configure flags @expr{--with-int-int@},
//! @expr{--with-long-int@}, and @expr{--with-long-long-int@}. The
//! default is to use the longest available integer type that fits
//! inside a pointer, which typically means that it's 64 bit on "true"
//! 64 bit architectures.
//!
//! @note
//! If Pike is compiled with the configure flag
//! @expr{--without-bignum@} (which is discouraged), then all
//! arithmetic operations will instead silently wrap around at these
//! limits.

//! Swaps the upper and lower byte in a word.
int(0..65535) swap_word(int(0..65535) i) {
  return ((i&255)<<8) | ((i&(255<<8))>>8);
}

//! Swaps the upper and lower word in a longword, and the upper and
//! lower bytes in the words. Simply put, the bytes are reversed.
int(0..4294967295) swap_long(int(0..4294967295) i) {
  return ((i&255)<<24) | ((i&(255<<8))<<8) |
    ((i&(255<<16))>>8) | ((i&(255<<24))>>24);
}

static class Inf {

  static constant neg = 0;
  static int __hash() { return 17; }
  static int(0..1) _equal(mixed arg) { return arg==this; }
  static int(0..1) _is_type(mixed type) { return (< "int", "object" >)[type]; }
  static mixed _random() { return this; }
  static mixed _sqrt() { return this; }
  // % == nan
  // & == nan
  static mixed `*(mixed ... args) {
    int n = neg;
    foreach(args, mixed arg)
      if(arg<0) n = !n;
    if(n) return ninf;
    return inf;
  }
  static mixed ``*(mixed ... args) { return `*(@args); }
  static mixed `+(mixed ... args) {
    foreach(args, mixed arg)
      if(arg==`-()) error("NaN\n");
    return this;
  }
  static mixed ``+(mixed ... args) { return ``+(@args); }
  static mixed `-(mixed ... args) {
    if(!sizeof(args)) {
      if(neg) return inf;
      return ninf;
    }
    foreach(args, mixed arg)
      if(arg==inf || arg==ninf) error("NaN\n");
    return this;
  }
  static mixed ``-(mixed arg) {
    if(arg==inf || arg==ninf) error("NaN\n");
    return this;
  }
  static int(0..1) `<(mixed arg) {
    if(arg==this) return 0;
    return neg;
  }
  static int(0..1) `>(mixed arg) {
    if(arg==this) return 0;
    return !neg;
  }
  static mixed `~() { return `-(); }
  static mixed cast(string to) {
    switch(to) {
    case "string":
      return "inf";
    case "float":
      return Math.inf;
    default:
      error("Can not cast to %O.\n", to);
    }
  }
  static string _sprintf(int t) {
    return t=='O' && (neg?"-":"")+"Int.inf";
  }
}

class NInf {
  inherit Inf;
  constant neg = 1;
}

static Inf ninf = NInf();

//! An infinite number.
Inf inf = Inf();
