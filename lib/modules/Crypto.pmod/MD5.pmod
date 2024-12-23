#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.MD5)

//! MD5 is a message digest function constructed by Ronald Rivest, and
//! is described in @rfc{1321@}. It outputs message digests of 128
//! bits, or 16 octets.

inherit Nettle.MD5;

@Pike.Annotations.Implements(Crypto.Hash);

Standards.ASN1.Types.Identifier pkcs_hash_id()
{
  return Standards.PKCS.Identifiers.md5_id;
}

//! This is a convenience alias for @[Nettle.crypt_md5()],
//! that uses the same API as the other hashes.
//!
//! @note
//!   The @[rounds] parameter is currently ignored.
//!   For forward compatibility, either leave out,
//!   or specify as @expr{1000@}.
//!
//! @seealso
//!   @[Nettle.Hash()->crypt_hash()], @[crypt_md5()]
string(7bit) crypt_hash(string(8bit) password, string(7bit) salt,
                        int(0..)|void rounds)
{
  string(8bit) orig_password = password;
  password = "censored";
  return Nettle.crypt_md5(orig_password, salt);
}

//! This is an implementation of the Apache password hashing algorithm
//! typically denoted as @expr{"apr1"@}.
//!
//! @param password
//!   Password to hash.
//!
//! @param salt
//!   8 bytes of salt.
//!
//! @param round
//!   Number of algorithm rounds. Default: @expr{1000@}.
//!
//! @seealso
//!   @[Nettle.Hash()->crypt_hash()], @[crypt_md5()],
//!   @url{https://fr.wikipedia.org/wiki/APR1@}
string(7bit) crypt_hash_apr1(string(8bit) password, string(7bit) salt,
                             int(0..) rounds = 1000)
{
  string(8bit) orig_password = password;
  password = "censored";
  String.secure(orig_password);
  int max = sizeof(orig_password);

  salt = salt[..7];
  State ctx = State();
  ctx->update(orig_password)->update(salt)->update(orig_password);
  string(8bit) digest = ctx->digest();

  ctx->update(orig_password)->update("$apr1$")->update(salt);

  int i;
  for (i = max; i > 0; i -= 16)
  {
    ctx->update(digest[.. min(16, i) - 1]);
  }

  for (i = max; i > 0; i >>= 1)
  {
    ctx->update((i & 1) ? "\0" : orig_password[0..0]);
  }

  digest = ctx->digest();

  for (i = 0; i < rounds; i++)
  {
    ctx->update((i & 1) ? orig_password : digest);
    if (i % 3)
    {
      ctx->update(salt);
    }
    if (i % 7)
    {
      ctx->update(orig_password);
    }
    ctx->update((i & 1) ? digest : orig_password);
    digest = ctx->digest();
  }

  /* And now time for some pointless shuffling of the result
   * (as per tradition).
   *
   * Note that the string is built in reverse in order to be able
   * to use MIME.encode_base64() for the actual encoding and the
   * code points are then swapped.
   */
  string(8bit) shuffled = "\0\0" +
    (string(8bit))(digest[({ 11, 4, 10, 5, 3, 9, 15, 2,
                             8, 14, 1, 7, 13, 0, 6, 12 })[*]]);
  return [string(7bit)]
    replace(reverse(MIME.encode_base64(shuffled)[2..]),
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"/"",
            "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"/"");
}
