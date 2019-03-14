#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.SHA3_256)

//! SHA-3-256 is another hash function specified by NIST, intended as
//! a replacement for SHA-2. It outputs hash values of 256 bits, or 32
//! octets.

inherit Nettle.SHA3_256;
