/* Signature.pmod
 *
 */

#if constant(Crypto.Hash)
#define HASH Crypto.Hash
#else
#define HASH object
#endif

#pike __REAL_VERSION__

import Standards.ASN1.Types;

//! @decl string build_digestinfo(string msg, Crypto.Hash hash)
//! Construct a PKCS-1 digestinfo.
//! @param msg
//!   message to digest
//! @param hash
//!   crypto hash object such as @[Crypto.SHA1] or @[Crypto.MD5]
//! @seealso
//!   @[Crypto.RSA()->sign]
string(0..255) build_digestinfo(string(0..255) msg, HASH hash)
{
  if(!hash->asn1_id) error("Unknown ASN.1 id for hash.\n");
  Sequence digest_info = Sequence( ({ Sequence( ({ hash->asn1_id(),
                                                   Null() }) ),
				      OctetString(hash->hash(msg)) }) );
  return digest_info->get_der();
}

//! This is an ASN.1 structure from PKCS #10 v1.7 and others,
//! which represents a signed block of data.
//!
//! @seealso
//!   @[sign()], @[Standards.X509.sign_tbs()].
class Signed
{
  inherit Sequence;

  //! ASN.1 structure that has been signed.
  Object `tbs()
  {
    return elements[0];
  }

  //! ASN.1 structure to be signed.
  void `tbs=(object o)
  {
    elements[0] = o;
  }

  //! Signing algorithm that was used to sign with.
  Sequence `algorithm()
  {
    return elements[1];
  }

  //! Signing algorithm that will be used to sign with.
  //!
  //! Typically the result of @[Crypto.Sign()->pkcs_signature_algorithm()].
  //!
  //! @seealso
  //!   @[sign()]
  void `algorithm=(Sequence a)
  {
    elements[1] = a;
  }

  //! The signature.
  BitString `signature()
  {
    return elements[2];
  }

  //! The resulting signature from signing the DER of @[tbs] with @[algorithm].
  //!
  //! Typically the result of @[Crypto.Sign()->pkcs_sign()].
  //!
  //! @seealso
  //!   @[sign()]
  void `signature=(BitString s)
  {
    elements[2] = s;
  }

  //! Sign @[tbs] with the provided @[sign] and @[hash].
  //!
  //! Sets @[algorithm] and @[signature].
  //!
  //! @returns
  //!   Returns the @[Signed] object.
  this_program sign(Crypto.Sign sign, Crypto.Hash hash)
  {
    elements[1] = sign->pkcs_signature_algorithm_id(hash);
    elements[2] = BitString(sign->pkcs_sign(tbs->get_der(), hash));
    return this;
  }

  protected void create(Sequence|void s)
  {
    elements = ({ Null(), Null(), Null() });
    if (s) {
      if ((s->type_name != "SEQUENCE") || (sizeof(s->elements) != 3)) {
	error("Invalid arguments to Standards.PKCS.Signature.Signed.");
      }
      elements = s->elements;
    }
  }
}

//! Generic PKCS signing.
//!
//! @param tbs
//!   @[Standards.ASN1] structure to be signed.
//!
//! @param sign
//!   Signature to use. Must have a private key set.
//!
//! @param hash
//!   Hash algorithm to use for the signature.
//!   Must be valid for the signature algorithm.
//!
//! @returns
//!   Returns a @[Standards.ASN1.Types.Sequence] with
//!   the signature.
Signed sign(Sequence tbs, Crypto.Sign sign, Crypto.Hash hash)
{
  Signed res = Signed();
  res->tbs = tbs;
  return res->sign(sign, hash);
}
