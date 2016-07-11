#pike __REAL_VERSION__
#pragma strict_types
#require constant(System.hw_random)

//! This class implements a random generator based on the hardware
//! generator available on some system. Currently only supports Intel
//! RDRAND CPU extension.
//!
//! In case of a process fork the generators in the two processes will
//! generate independent random values.

inherit Builtin.RandomInterface;

string(8bit) random_string(int(0..) length)
{
  return System.hw_random(length);
}
