#pike __REAL_VERSION__
#pragma strict_types

//! SHA256 is another hash function specified by NIST, intended as a
//! replacement for @[SHA1], generating larger digests. It outputs hash
//! values of 256 bits, or 32 octets.

#if constant(Nettle) && constant(Nettle.SHA256)

inherit Nettle.SHA256;

Standards.ASN1.Types.Identifier asn1_id()
{
  return Standards.PKCS.Identifiers.sha256_id;
}

#else
constant this_program_does_not_exist=1;
#endif
