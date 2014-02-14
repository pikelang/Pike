#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.SHA384)

//! SHA384 is another hash function specified by NIST, intended as a
//! replacement for @[SHA1], generating larger digests. It outputs hash
//! values of 384 bits, or 48 octets.

inherit Nettle.SHA384;

Standards.ASN1.Types.Identifier asn1_id()
{
  return Standards.PKCS.Identifiers.sha384_id;
}
