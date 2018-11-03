#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.SHA512_256)

//! SHA512/256 is another hash function specified by NIST. It is
//! similar to @[SHA512] except it uses a different initialization and
//! the output is truncated to 256 bits, or 32 octets.

inherit Nettle.SHA512_256;

Standards.ASN1.Types.Identifier pkcs_hash_id()
{
  return Standards.PKCS.Identifiers.sha512_256_id;
}
