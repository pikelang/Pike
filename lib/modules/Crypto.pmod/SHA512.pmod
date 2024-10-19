#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.SHA512)

//! SHA512 is another hash function specified by NIST, intended as a
//! replacement for @[SHA1], generating larger digests. It outputs hash
//! values of 512 bits, or 64 octets.

inherit Nettle.SHA512;

@Pike.Annotations.Implements(Crypto.Hash);

Standards.ASN1.Types.Identifier pkcs_hash_id()
{
  return Standards.PKCS.Identifiers.sha512_id;
}

protected constant hmac_jwa_id = "HS512";
