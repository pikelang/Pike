#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.SHA3_384)

//! SHA-3-386 is another hash function specified by NIST, intended as
//! a replacement for SHA-2. It outputs hash values of 256 bits, or 48
//! octets.

inherit Nettle.SHA3_384;
