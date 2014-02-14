#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.BLOWFISH)

//! BLOWFISH is a block cipher designed by Bruce Schneier. It uses a
//! block size of 64 bits (8 octets), and a variable key size, up to
//! 448 bits. It has some weak keys.

inherit Nettle.BLOWFISH;
