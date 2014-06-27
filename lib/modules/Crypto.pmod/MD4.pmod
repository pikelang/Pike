#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.MD4)

//! MD4 is a message digest function constructed by Ronald Rivest, and
//! is described in RFC 1320. It outputs message digests of 128 bits,
//! or 16 octets.

inherit Nettle.MD4;

Standards.ASN1.Types.Identifier pkcs_hash_id()
{
  return Standards.PKCS.Identifiers.md4_id;
}
