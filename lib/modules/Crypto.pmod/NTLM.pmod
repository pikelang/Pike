#pike __REAL_VERSION__

//!
//! NT Lan Manager authentication protocol primitives.
//!
//! @note
//!   These functions use obsolete crypto primitives in weak and
//!   questionable ways, and should be avoided if possible. They
//!   are only intended to be used for interop with old code.
//!
//! @seealso
//!   [MS-NLMP]:@url{http://download.microsoft.com/download/9/5/e/95ef66af-9026-4bb0-a41d-a4f81802d92c/[ms-nlmp].pdf@}
//!

protected string(8bit) UNICODE(string s)
{
  return Charset.encoder("utf16le")->feed(s)->drain();
}

protected string(8bit) DESL(string(8bit) key, string(8bit) data)
{
  if (sizeof(key) != 16)
    error("Unexpected key length: %O (%d)\n", key, sizeof(key));
  if (sizeof(data) != 8)
    error("Unexpected key length: %O (%d)\n", data, sizeof(data));
  return .DES()->set_encrypt_key(.DES.fix_parity(key[..6]), 1)->crypt(data) +
    .DES()->set_encrypt_key(.DES.fix_parity(key[7..13]), 1)->crypt(data) +
    .DES()->set_encrypt_key(.DES.fix_parity(key[14..15] + ("\0" * 5)), 1)->
    crypt(data);
}

constant LM_MAGIC = "KGS!@#$%";

//! Lan-Manager One-Way Function version 1.
//!
//! This function is also known as LM hash.
string(8bit) LMOWFv1(string Passwd, string User, string UserDom)
{
  Passwd = upper_case(Passwd) + ("\0"*14);
  // FIXME: Convert Passwd to the system charset.
  // NB: We MUST force the set_encrypt_key() as it may very well be
  //     a weak key.
  return .DES()->set_encrypt_key(.DES.fix_parity(Passwd[..6]), 1)->
    crypt(LM_MAGIC) +
    .DES()->set_encrypt_key(.DES.fix_parity(Passwd[7..13]), 1)->
    crypt(LM_MAGIC);
}

//! NT One-Way Function version 1.
string(8bit) NTOWFv1(string Passwd, string User, string UserDom)
{
  return .MD4.hash(UNICODE(Passwd));
}

//! Session Base Key for NTLMv1.
string(8bit) SBKv1(string Passwd, string User, string UserDom)
{
  return .MD4.hash(NTOWFv1(Passwd, User, UserDom));
}

//! NT One-Way Function version 2.
string(8bit) NTOWFv2(string Passwd, string User, string UserDom)
{
  return .MD5.HMAC(.MD4.hash(UNICODE(Passwd)))->
    update(UNICODE(upper_case(User) + UserDom))->digest();
}

//! Lan-Manager One-Way Function version 2.
//!
//! This function is identical to @[NTOWFv2()].
string(8bit) LMOWFv2(string Passwd, string User, string UserDom)
{
  return NTOWFv2(Passwd, User, UserDom);
}
