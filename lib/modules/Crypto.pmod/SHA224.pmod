#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.SHA224)

//! SHA224 is another hash function specified by NIST, intended as a
//! replacement for @[SHA1], generating larger digests. It outputs hash
//! values of 224 bits, or 28 octets.

inherit Nettle.SHA224;
