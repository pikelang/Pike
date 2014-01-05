#pike __REAL_VERSION__
#pragma strict_types

//! SHA512 is another hash function specified by NIST, intended as a
//! replacement for @[SHA1], generating larger digests. It outputs hash
//! values of 512 bits, or 64 octets.

#if constant(Nettle) && constant(Nettle.SHA512)

inherit Nettle.SHA512;

Standards.ASN1.Types.Identifier asn1_id()
{
  return Standards.PKCS.Identifiers.sha512_id;
}

#else
constant this_program_does_not_exist=1;
#endif
