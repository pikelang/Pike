#pike __REAL_VERSION__
#pragma strict_types

//! CAST-128 is a block cipher, specified in RFC 2144. It uses a 64
//! bit (8 octets) block size, and a variable key size of up to 128
//! bits.

#if constant(Nettle) && constant(Nettle.CAST128)

inherit Nettle.CAST128;

#else
constant this_program_does_not_exist=1;
#endif
