#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.CAST128)

//! CAST-128 is a block cipher, specified in RFC 2144. It uses a 64
//! bit (8 octets) block size, and a variable key size of up to 128
//! bits.

inherit Nettle.CAST128;
