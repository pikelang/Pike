#pike __REAL_VERSION__
#pragma strict_types

//! SHA1 is a hash function specified by NIST (The U.S. National
//! Institute for Standards and Technology). It outputs hash values of
//! 160 bits, or 20 octets.

#if constant(Nettle) && constant(Nettle.SHA1_Info)

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.SHA1_Info;
inherit .Hash;

.HashState `()() { return Nettle.SHA1_State(); }

#else
constant this_program_does_not_exist=1;
#endif
