#pike __REAL_VERSION__
#pragma strict_types

//! The Camellia 128-bit block cipher.

#if constant(Nettle) && constant(Nettle.CAMELLIA)

inherit Nettle.CAMELLIA;

#else
constant this_program_does_not_exist=1;
#endif
