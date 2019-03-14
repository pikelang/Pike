#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.SHA3_224)

//! SHA-3-224 is another hash function specified by NIST, intended as
//! a replacement for SHA-2. It outputs hash values of 224 bits, or 28
//! octets.

inherit Nettle.SHA3_224;
