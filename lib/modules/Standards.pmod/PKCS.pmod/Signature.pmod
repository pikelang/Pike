/* Signature.pmod
 *
 */

#pike __REAL_VERSION__

//! Construct a PKCS-1 digestinfo
//! @param msg
//!   message to digest
//! @param hash
//!   crypto hash object such as @[Crypto.sha] or @[Crypto md5]
string build_digestinfo(string msg, object hash)
{
  string d = hash->update(msg)->digest();
  string id = hash->identifier();

  return sprintf("%c%c%c%c%c%c%s%c%c%c%c%s",
		 0x30, strlen(id) + strlen(d) + 8, 0x30, strlen(id) + 4,
		 0x06, strlen(id), id, 0x05, 0x00, 0x04, strlen(d), d);
}
