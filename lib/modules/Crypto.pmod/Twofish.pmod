#pike __REAL_VERSION__
#pragma strict_types

//! Another @[AES] finalist, this one designed by Bruce Schneier and
//! others.

#if constant(Nettle) && constant(Nettle.Twofish)

inherit Nettle.Twofish;

#else
constant this_program_does_not_exist=1;
#endif
