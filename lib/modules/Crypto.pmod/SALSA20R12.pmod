#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.SALSA20R12)

//! The @[SALSA20] stream cipher reduced to just 12 rounds.

inherit Nettle.SALSA20R12;
