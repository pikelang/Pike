#pike __REAL_VERSION__

//! Returns the parity of the integer @[value]. If the
//! parity is odd 1 is returned. If it is even 0 is
//! returned.
int(0..1) parity(int(0..) value) {
  if(value<0) error("Parity can not determined for negative values.\n");
  return Gmp.mpz(value)->popcount()&1;
}
