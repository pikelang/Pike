#pike __REAL_VERSION__
#pragma strict_types

//! The @[SALSA20] stream cipher reduced to just 12 rounds.

#if constant(Nettle) && constant(Nettle.SALSA20R12)

inherit Nettle.SALSA20R12;

#else
constant this_program_does_not_exist=1;
#endif
