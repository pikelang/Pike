//! Handling of Certificate Signing Requests (PKCS-10, RFC 2314, RFC 2986)

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

// FIXME: Mark as deprecated!
constant CSR_Attributes = CRIAttributes;

//! CertificationRequestInfo
//!
//! This is the data that is signed by @[sign_cri()].
class CRI
{
  inherit Sequence;

  //!
  void `version=(int v)
  {
    der = UNDEFINED;
    if (v != 1) {
      error("Only version 1 is supported.\n");
    }
    elements[0] = Integer(v-1);
  }
  int `version()
  {
    return elements[0]->value + 1;
  }

  //! Certificate subject.
  void `subject=(Sequence i)
  {
    der = UNDEFINED;
    elements[1] = i;
  }
  Sequence `subject()
  {
    return elements[1];
  }

  //! Public key information.
  void `keyinfo=(Sequence ki)
  {
    der = UNDEFINED;
    elements[2] = ki;
  }
  Sequence `keyinfo()
  {
    return elements[2];
  }

  //! Subject attributes.
  void `attributes=(Set|CRIAttributes attrs)
  {
    der = UNDEFINED;
    if ((attrs->type_name != "SET") &&
	(attrs->type_name != "CONSTRUCTED")) {
      error("Invalid attributes: %O.\n", attrs->type_name);
    }
    if ((attrs->get_cls() != 2) || attrs->get_tag() || attrs->raw) {
      attrs = CRIAttributes(attrs->elements);
    }
    elements[3] = attrs;
  }
  CRIAttributes `attributes()
  {
    return elements[3];
  }

  protected void create(Sequence|void asn1)
  {
    if (asn1) {
      if ((asn1->type_name != "SEQUENCE") ||
	  (sizeof(asn1->elements) != 4) ||
	  (asn1[0]->type_name != "INTEGER") ||
	  asn1[0]->value) {
	error("Invalid CRI.");
      }
      elements = asn1->elements;
      attributes = elements[3];
    } else {
      elements = ({
	Integer(0),		// version (v1)
	Null,			// subject
	Null,			// subjectPKInfo
	CRIAttributes(({})),	// attributes
      });
    }
  }
}

//! Sign a @[CRI] to generate a Certificate Signing Request.
//!
//! @param cri
//!   CertificationRequestInfo to sign.
//!
//! @param sign
//!   Signature to use. Must have a private key set that matches
//!   the public key in the keyinfo in @[cri].
//!
//! @param hash
//!   Hash algorithm to use for the signature.
Sequence sign_cri(CRI cri, Crypto.Sign sign, Crypto.Hash hash)
{
  return Standards.PKCS.Signature.sign(cri, sign, hash);
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
