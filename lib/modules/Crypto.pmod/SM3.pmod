#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.SM3)

//! SM3 is a cryptographic hash function standard adopted by the
//! government of the People's Republic of China. It outputs hash
//! values of 256 bits, or 32 octets.

inherit Nettle.SM3;

Standards.ASN1.Types.Identifier pkcs_hash_id()
{
  return Standards.PKCS.Identifiers.sm3_id;
}
