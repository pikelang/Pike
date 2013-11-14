#pike __REAL_VERSION__
#pragma strict_types

//! SHA-3-224 is another hash function specified by NIST, intended as
//! a replacement for SHA-2. It outputs hash values of 224 bits, or 28
//! octets.

#if constant(Nettle.SHA3_224)

inherit Nettle.SHA3_224;

#else
constant this_program_does_not_exist=1;
#endif
