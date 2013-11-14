#pike __REAL_VERSION__
#pragma strict_types

//! SHA-3-512 is another hash function specified by NIST, intended as
//! a replacement for SHA-2. It outputs hash values of 512 bits, or 64
//! octets.

#if constant(Nettle.SHA3_512)

inherit Nettle.SHA3_512;

#else
constant this_program_does_not_exist=1;
#endif
