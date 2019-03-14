#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.SHA1)

//! SHA1 is a hash function specified by NIST (The U.S. National
//! Institute for Standards and Technology). It outputs hash values of
//! 160 bits, or 20 octets.

inherit Nettle.SHA1;

Standards.ASN1.Types.Identifier asn1_id()
{
  return Standards.PKCS.Identifiers.sha1_id;
}
