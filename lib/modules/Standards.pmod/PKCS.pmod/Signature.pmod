/* Signature.pmod
 *
 */

#pike __REAL_VERSION__

//! Construct a PKCS-1 digestinfo
//! @param msg
//!   message to digest
//! @param hash
//!   crypto hash object such as @[Crypto.SHA] or @[Crypto.MD5]
string build_digestinfo(string msg, Crypto.Hash hash)
{
  if(!hash->asn1_id) error("Unknown ASN.1 id for hash.\n");
  string d = hash->hash(msg);
  string id = hash->asn1_id();

  return sprintf("%c%c%c%c%c%c%s%c%c%c%c%s",
		 0x30, sizeof(id) + sizeof(d) + 8, 0x30, sizeof(id) + 4,
		 0x06, sizeof(id), id, 0x05, 0x00, 0x04, sizeof(d), d);
}
