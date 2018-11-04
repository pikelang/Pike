#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.SHA512_224)

//! SHA512/224 is another hash function specified by NIST. It is
//! similar to @[SHA512] except it uses a different initialization and
//! the output is truncated to 224 bits, or 28 octets.

inherit Nettle.SHA512_224;

Standards.ASN1.Types.Identifier pkcs_hash_id()
{
  return Standards.PKCS.Identifiers.sha512_224_id;
}
