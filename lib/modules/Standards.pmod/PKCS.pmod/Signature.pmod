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
string build_digestinfo(string msg, HASH hash)
{
  if(!hash->asn1_id) error("Unknown ASN.1 id for hash.\n");
  Sequence digest_info = Sequence( ({ Sequence( ({ hash->asn1_id(),
                                                   Null() }) ),
				      OctetString(hash->hash(msg)) }) );
  return digest_info->get_der();
}
