//! Handling of Certificate Signing Requests (PKCS-10, RFC 2986)

#pike __REAL_VERSION__
// #pragma strict_types
#require constant(Crypto.RSA)

import Standards.ASN1.Types;

//!
class CRIAttributes
{
  inherit .Certificate.Attributes;
  constant cls = 2;
  constant tag = 0;
}

//! Build a Certificate Signing Request.
//!
//! @param sign
//!   Signature algorithm for the certificate. Both private and
//!   public keys must be set.
//!
//! @param name
//!   The distinguished name for the certificate.
//!
//! @param attributes
//!   Attributes from PKCS #9 to add to the certificate.
//!
//! @param hash
//!   Hash algoritm to use for the CSR signature.
//!   Defaults to @[Crypto.SHA256].
//!
//! @note
//!   Prior to Pike 8.0 this function only supported signing
//!   with @[Crypto.RSA] and the default (and only) hash was
//!   @[Crypto.MD5].
Sequence build_csr(Crypto.Sign sign, Sequence name,
		   mapping(string:array(Object)) attributes,
		   Crypto.Hash|void hash)
{
  Sequence info = Sequence( ({ Integer(0),	// v1
			       name,
			       sign->pkcs_public_key(),
			       CRIAttributes(.Identifiers.attribute_ids,
					     attributes) }) );
  hash = hash || Crypto.SHA256;
  return Sequence( ({ info,
		      sign->pkcs_signature_algorithm_id(hash),
		      BitString(sign->pkcs_sign(info->get_der(), hash))
		   }) );
}
