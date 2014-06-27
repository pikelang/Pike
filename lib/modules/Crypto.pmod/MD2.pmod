#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.MD2)

//! MD2 is a message digest function constructed by Burton Kaliski,
//! and is described in RFC 1319. It outputs message digests of 128
//! bits, or 16 octets.

inherit Nettle.MD2;

Standards.ASN1.Types.Identifier pkcs_hash_id()
{
  return Standards.PKCS.Identifiers.md2_id;
}
