#pike __REAL_VERSION__
#pragma strict_type

//! Returns the parity of the integer @[value]. If the
//! parity is odd 1 is returned. If it is even 0 is
//! returned.
int(0..1) parity(int(0..) value) {
  return String.count(sprintf("%b",value),"1")&1;
}
