#pike __REAL_VERSION__
#pragma strict_types

//! SHA384 is another hash function specified by NIST, intended as a
//! replacement for @[SHA1], generating larger digests. It outputs hash
//! values of 384 bits, or 48 octets.

#if constant(Nettle) && constant(Nettle.SHA384)

inherit Nettle.SHA384;

Standards.ASN1.Types.Identifier asn1_id()
{
  return Standards.PKCS.Identifiers.sha384_id;
}

#else
constant this_program_does_not_exist=1;
#endif
