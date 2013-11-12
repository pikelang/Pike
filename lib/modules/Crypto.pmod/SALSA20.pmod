#pike __REAL_VERSION__
#pragma strict_types

//! The Camellia 128-bit block cipher.

#if constant(Nettle) && constant(Nettle.SALSA20)

inherit Nettle.SALSA20;

#else
constant this_program_does_not_exist=1;
#endif
