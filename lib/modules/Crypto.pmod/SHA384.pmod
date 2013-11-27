#pike __REAL_VERSION__
#pragma strict_types

//! SHA384 is another hash function specified by NIST, intended as a
//! replacement for @[SHA1], generating larger digests. It outputs hash
//! values of 384 bits, or 48 octets.

#if constant(Nettle) && constant(Nettle.SHA384_Info)

// NOTE: Depends on the order of INIT invocations.
inherit Nettle.SHA384_Info;
inherit .Hash;

.HashState `()() { return Nettle.SHA384_State(); }

// Standards.ASN1.Types.Identifier(2,16,840,1,101,3,4,2,2)->get_der();
string asn1_id() { return "`\206H\1e\3\4\2\2"; }

#else
constant this_program_does_not_exist=1;
#endif
