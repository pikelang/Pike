#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.SHA1)

//! SHA1 is a hash function specified by NIST (The U.S. National
//! Institute for Standards and Technology). It outputs hash values of
//! 160 bits, or 20 octets.

inherit Nettle.SHA1;

Standards.ASN1.Types.Identifier pkcs_hash_id()
{
  return Standards.PKCS.Identifiers.sha1_id;
}

//! @module HMAC

//! @ignore
protected class _HMAC
{
  //! @endignore

  inherit ::this_program;

  //! crypt_sha1() from NetBSD.
  string(7bit) crypt_hash(string(8bit) password, string(7bit) salt, int rounds)
  {
    State s = `()(password);
    password = "CENSORED";
    string(8bit) checksum = salt + "$sha1$" + rounds;
    while (rounds--) {
      checksum = s(checksum);
    }

    // Some stupid shuffling for no specific reason...
    string(8bit) result = "";
    foreach(({ 2, 1, 0, 5, 4, 3, 8, 7,
	       6, 11, 10, 9, 14, 13, 12, 17,
	       16, 15, 0, 19, 18 }), int i) {
      result += checksum[i..i];
    }

    return MIME.encode_crypt64(result);
  }

  //! @ignore
}
//! @endignore
