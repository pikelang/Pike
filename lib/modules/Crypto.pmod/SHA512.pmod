#pike __REAL_VERSION__
#pragma strict_types

//! SHA512 is another hash function specified by NIST, intended as a
//! replacement for @[SHA1], generating larger digests. It outputs hash
//! values of 512 bits, or 32 octets.

#if constant(Nettle) && constant(Nettle.SHA512_Info)

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.SHA512_Info;
inherit .Hash;

.HashState `()() { return Nettle.SHA512_State(); }

// Standards.ASN1.Types.Identifier(2,16,840,1,101,3,4,2,3)->get_der();
string asn1_id() { return "`\206H\1e\3\4\2\3"; }

#else
constant this_program_does_not_exist=1;
#endif
