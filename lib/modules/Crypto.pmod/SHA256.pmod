#pike __REAL_VERSION__
#pragma strict_types

//! SHA256 is another hash function specified by NIST, intended as a
//! replacement for @[SHA1], generating larger digests. It outputs hash
//! values of 256 bits, or 32 octets.

#if constant(Nettle) && constant(Nettle.SHA256)

inherit Nettle.SHA256;

// id-sha256
//   FROM RFC 4055 {
//     joint-iso-itu-t(2) country(16) us(840) organization(1) gov(101)
//     csor(3) nistalgorithm(4) hashalgs(2) 1
//   }
//
// Standards.ASN1.Types.Identifier(2,16,840,1,101,3,4,2,1)->get_der();
string asn1_id() { return "`\206H\1e\3\4\2\1"; }

#else
constant this_program_does_not_exist=1;
#endif
