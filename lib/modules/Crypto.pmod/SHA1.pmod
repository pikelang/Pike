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

// id-sha1    OBJECT IDENTIFIER ::= {
//   iso(1) identified-organization(3) oiw(14) secsig(3) algorithms(2) 26
// }
//
// Standards.ASN1.Types.Identifier(1,3,14,3,2,26)->get_der();
string asn1_id() { return "+\16\3\2\32"; }

#else
constant this_program_does_not_exist=1;
#endif
