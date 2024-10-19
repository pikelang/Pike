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

//! Swaps the upper and lower byte in a word.
//!
//! @seealso
//!   @[swap_long()]
int(0..65535) swap_word(int(0..65535) i) {
  return ((i&255)<<8) | ((i&(255<<8))>>8);
}

//! Swaps the upper and lower word in a longword, and the upper and
//! lower bytes in the words. Simply put, the bytes are reversed.
//!
//! @seealso
//!   @[swap_word()]
int(0..4294967295) swap_long(int(0..4294967295) i) {
  return ((i&255)<<24) | ((i&(255<<8))<<8) |
    ((i&(255<<16))>>8) | ((i&(255<<24))>>24);
}

//! Reverses the order of the low order @[bits] number of bits
//! of the value @[value].
//!
//! @note
//!   Any higher order bits of the value will be cleared.
//!   The returned value will thus be unsigned.
//!
//! @seealso
//!   @[reverse()], @[swap_word()], @[swap_long()]
int(0..) reflect(int value, int(0..) bits)
{
  int aligned_bits = bits;
  if (bits & (bits-1)) {
    // Find the closest larger even power of two.
    aligned_bits <<= 1;
    while (aligned_bits & (aligned_bits-1)) {
      aligned_bits -= (aligned_bits & ~(aligned_bits-1));
    }
  }
  bits = aligned_bits - bits;
  // Perform pair-wise swapping of bit-sequences.
  int mask = (1<<aligned_bits)-1;
  int filter = mask;
  while (aligned_bits >>= 1) {
    filter ^= filter>>aligned_bits;
    value = (value & filter)>>aligned_bits |
      (value & (filter^mask))<<aligned_bits;
  }

  // Adjust the returned value in case we've swapped more bits
  // than needed. We then have junk in the lowest order bits.
  return value>>bits;
}

//! The type of @[Int.inf]. Do not create more instances of this.
class Inf {

  protected constant neg = 0;
  protected int __hash() { return 17; }
  protected int(0..1) _equal(mixed arg) {
    if(neg && arg==-Math.inf) return 1;
    if(!neg && arg==Math.inf) return 1;
    return arg==this;
  }
  protected int(0..1) _is_type(mixed type) { return (< "int", "object" >)[type]; }
  protected mixed _random(function rnd_string, function rnd) {
    if (neg) return 0;
    return this;
  }
  protected mixed _sqrt() { return this; }
  // % == nan
  // & == nan
  protected mixed `*(mixed arg) {
    int n = neg;
    if(arg<0) n = !n;
    if(n) return ninf;
    return inf;
  }
  protected mixed ``*(mixed arg) { return `*(arg); }
  protected mixed `+(mixed arg) {
    if(arg==`-()) error("NaN\n");
    return this;
  }
  protected mixed ``+(mixed arg) { return ``+(arg); }
  protected mixed `-(void|mixed arg) {
    if(!query_num_arg()) {
      if(neg) return inf;
      return ninf;
    }
    if(arg==inf || arg==ninf) error("NaN\n");
    return this;
  }
  protected mixed ``-(mixed arg) {
    if(arg==inf || arg==ninf) error("NaN\n");
    return this;
  }
  protected int(0..1) `<(mixed arg) {
    if(arg==this) return 0;
    return neg;
  }
  protected int(0..1) `>(mixed arg) {
    if(arg==this) return 0;
    return !neg;
  }
  protected mixed `~() { return `-(); }
  protected mixed `<<(mixed arg) {
    if(arg<0) error("Got negative shift count.\n");
    return this;
  }
  protected mixed ``<<(mixed arg) {
    if(arg<0) return ninf;
    return inf;
  }
  protected mixed `>>(mixed arg) {
    if(arg<0) error("Got negative shift count.\n");
    return this;
  }
  protected mixed ``>>(mixed arg) {
    return 0;
  }
  protected mixed cast(string to) {
    switch(to) {
    case "string":
      return "inf";
    case "float":
      return Math.inf;
    default:
      return UNDEFINED;
    }
  }
  protected string _sprintf(int t) {
    return t=='O' && (neg?"-":"")+"Int.inf";
  }
}

class NInf {
  inherit Inf;
  constant neg = 1;
}

//! An object that behaves like negative infinity.
Inf ninf = NInf();

//! An object that behaves like positive infinity.
Inf inf = Inf();
