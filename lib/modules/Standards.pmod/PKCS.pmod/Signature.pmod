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
Sequence sign(Sequence tbs, Crypto.Sign sign, Crypto.Hash hash)
{
  return Sequence( ({ tbs,
		      sign->pkcs_signature_algorithm_id(hash),
		      BitString(sign->pkcs_sign(tbs->get_der(), hash))
		   }) );
}
