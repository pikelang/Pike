#pike __REAL_VERSION__
#pragma strict_types

//! Password handling.
//!
//! This module handles generation and verification of
//! password hashes.
//!
//! @seealso
//!   @[verify()], @[hash()], @[crypt()]

//! Verify a password against a hash.
//!
//! This function attempts to support most common
//! password hashing schemes. The @[hash] can be on any
//! of the following formats.
//!
//! LDAP-style (RFC2307) hashes:
//! @string
//!   @value "{SHA}XXXXXXXXXXXXXXXXXXXXXXXXXXXX"
//!     The @expr{XXX@} string is taken to be a @[MIME.encode_base64]
//!     @[SHA1] hash of the password. Source: OpenLDAP FAQ
//!     @url{http://www.openldap.org/faq/data/cache/347.html@}.
//!
//!   @value "{SSHA}XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
//!     The @expr{XXX@} string is taken to be a @[MIME.encode_base64]
//!     string in which the first 20 chars are an @[SHA1] hash and the
//!     remaining chars the salt. The input for the hash is the password
//!     concatenated with the salt. Source: OpenLDAP FAQ
//!     @url{http://www.openldap.org/faq/data/cache/347.html@}.
//!
//!   @value "{MD5}XXXXXXXXXXXXXXXXXXXXXXXX"
//!     The @expr{XXX@} string is taken to be a @[MIME.encode_base64] @[MD5]
//!     hash of the password. Source: OpenLDAP FAQ
//!     @url{http://www.openldap.org/faq/data/cache/418.html@}.
//!
//!   @value "{SMD5}XXXXXXXXXXXXXXXXXXXXXXXXXXXX"
//!     The @expr{XXX@} string is taken to be a @[MIME.encode_base64]
//!     string in which the first 16 chars are an @[MD5] hash and the
//!     remaining chars the salt. The input for the hash is the password
//!     concatenated with the salt. Source: OpenLDAP FAQ
//!     @url{http://www.openldap.org/faq/data/cache/418.html@}.
//!
//!   @value "{CRYPT}XXXXXXXXXXXXX"
//!     The @expr{XX@} string is taken to be a crypt(3C)-style hash.
//!     This is the same thing as passing the @expr{XXX@} string without
//!     any preceding method name within @expr{{...}@}. I.e. it's
//!     interpreted according to the crypt-style hashes below.
//! @endstring
//!
//! Crypt-style hashes:
//! @string
//!   @value "$6$SSSSSSSSSSSSSSSS$XXXXXXXXXXXXXXXXXXXXXX"
//!     The string is interpreted according to the
//!     "Unix crypt using SHA-256 and SHA-512" standard
//!     Version 0.4 2008-4-3, where @expr{SSSSSSSSSSSSSSSS@}
//!     is up to 16 characters of salt, and the string @expr{XXX@}
//!     the result of @[SHA512.crypt_hash()] with @expr{5000@}
//!     rounds. Source: Unix crypt using SHA-256 and SHA-512
//!     @url{http://www.akkadia.org/drepper/SHA-crypt.txt@}
//!
//!   @value "$6$rounds=RR$SSSSSSSSSSSSSSSS$XXXXXXXXXXXXXXXXXXXXXX"
//!     This is the same algorithm as the one above, but with the
//!     number of rounds specified by @expr{RR@} in decimal. Note
//!     that the number of rounds is clamped to be within
//!     @expr{1000@} and @expr{999999999@} (inclusive).
//!     Source: Unix crypt using SHA-256 and SHA-512
//!     @url{http://www.akkadia.org/drepper/SHA-crypt.txt@}
//!
//!   @value "$5$SSSSSSSSSSSSSSSS$XXXXXXXXXXXXXXXXXXXXXX"
//!     The string is interpreted according to the
//!     "Unix crypt using SHA-256 and SHA-512" standard
//!     Version 0.4 2008-4-3, where @expr{SSSSSSSSSSSSSSSS@}
//!     is up to 16 characters of salt, and the string @expr{XXX@}
//!     the result of @[SHA256.crypt_hash()] with @expr{5000@}
//!     rounds. Source: Unix crypt using SHA-256 and SHA-512
//!     @url{http://www.akkadia.org/drepper/SHA-crypt.txt@}
//!
//!   @value "$5$rounds=RR$SSSSSSSSSSSSSSSS$XXXXXXXXXXXXXXXXXXXXXX"
//!     This is the same algorithm as the one above, but with the
//!     number of rounds specified by @expr{RR@} in decimal. Note
//!     that the number of rounds is clamped to be within
//!     @expr{1000@} and @expr{999999999@} (inclusive).
//!     Source: Unix crypt using SHA-256 and SHA-512
//!     @url{http://www.akkadia.org/drepper/SHA-crypt.txt@}
//!
//!   @value "$1$SSSSSSSS$XXXXXXXXXXXXXXXXXXXXXX"
//!     The string is interpreted according to the GNU libc2 extension
//!     of @expr{crypt(3C)@} where @expr{SSSSSSSS@} is up to 8 chars of
//!     salt and the @expr{XXX@} string is an @[MD5]-based hash created
//!     from the password and the salt. Source: GNU libc
//!     @url{http://www.gnu.org/software/libtool/manual/libc/crypt.html@}.
//!
//!   @value "XXXXXXXXXXXXX"
//!     The @expr{XXX@} string (which doesn't begin with @expr{"{"@}) is
//!     taken to be a password hashed using the classic unix
//!     @expr{crypt(3C)@} function. If the string contains only chars
//!     from the set @expr{[a-zA-Z0-9./]@} it uses DES and the first two
//!     characters as salt, but other alternatives might be possible
//!     depending on the @expr{crypt(3C)@} implementation in the
//!     operating system.
//!
//!   @value ""
//!     The empty password hash matches all passwords.
//! @endstring
//!
//! @returns
//!   Returns @expr{1@} on success, and @expr{0@} (zero) otherwise.
//!
//! @note
//!   This function was added in Pike 7.8.755.
//!
//! @seealso
//!   @[hash()], @[predef::crypt()]
int verify(string(8bit) password, string(8bit) hash)
{
  if (hash == "") return 1;

  // Detect the password hashing scheme.
  // First check for an LDAP-style marker.
  string scheme = "crypt";
  sscanf(hash, "{%s}%s", scheme, hash);
  // NB: RFC2307 proscribes lower case schemes, while
  //     in practise they are usually in upper case.
  switch(lower_case(scheme)) {
  case "md5":	// RFC 2307
  case "smd5":
    hash = MIME.decode_base64(hash);
    password += hash[16..];
    hash = hash[..15];
    return Crypto.MD5.hash(password) == hash;

  case "sha":	// RFC 2307
  case "ssha":
    // SHA1 and Salted SHA1.
    hash = MIME.decode_base64(hash);
    password += hash[20..];
    hash = hash[..19];
    return Crypto.SHA1.hash(password) == hash;

  case "crypt":	// RFC 2307
    // First try the operating systems crypt(3C),
    // since it might support more schemes than we do.
    if ((hash == "") || crypt(password, hash)) return 1;
    if (hash[0] != '$') {
      if (hash[0] == '_') {
	// FIXME: BSDI-style crypt(3C).
      }
      return 0;
    }

    // Then try our implementations.
    sscanf(hash, "$%s$%s$%s", scheme, string(8bit) salt, string(8bit) hash);
    if( !salt || !hash ) return 0;
    int rounds = UNDEFINED;
    if (has_prefix(salt, "rounds=")) {
      sscanf(salt, "rounds=%d", rounds);
      sscanf(hash, "%s$%s", salt, hash);
    }
    switch(scheme) {
    case "1":	// crypt_md5
      return Nettle.crypt_md5(password, salt) == [string(7bit)]hash;

    case "2":	// Blowfish (obsolete)
    case "2a":	// Blowfish (possibly weak)
    case "2x":	// Blowfish (weak)
    case "2y":	// Blowfish (stronger)
      break;

    case "3":	// MD4 NT LANMANAGER (FreeBSD)
      break;

      // cf http://www.akkadia.org/drepper/SHA-crypt.txt
    case "5":	// SHA-256
      return Crypto.SHA256.crypt_hash(password, salt, rounds) ==
        [string(7bit)]hash;
#if constant(Crypto.SHA512)
    case "6":	// SHA-512
      return Crypto.SHA512.crypt_hash(password, salt, rounds) ==
        [string(7bit)]hash;
#endif
    }
    break;
  }
  return 0;
}

//! Generate a hash of @[password] suitable for @[verify()].
//!
//! @param password
//!   Password to hash.
//!
//! @param scheme
//!   Password hashing scheme. If not specified the strongest available
//!   will be used.
//!
//!   If an unsupported scheme is specified an error will be thrown.
//!
//!   Supported schemes are:
//!
//!   Crypt(3C)-style:
//!   @string
//!     @value UNDEFINED
//!     @value "crypt"
//!     @value "{crypt}"
//!       Use the strongest crypt(3C)-style hash that is supported.
//!
//!     @value "6"
//!     @value "$6$"
//!       @[SHA512.crypt_hash()] with 96 bits of salt and a default
//!       of @expr{5000@} rounds.
//!
//!     @value "5"
//!     @value "$5$"
//!       @[SHA256.crypt_hash()] with 96 bits of salt and a default
//!       of @expr{5000@} rounds.
//!
//!     @value "1"
//!     @value "$1$"
//!       @[MD5.crypt_hash()] with 48 bits of salt and @expr{1000@} rounds.
//!
//!     @value ""
//!       @[predef::crypt()] with 12 bits of salt.
//!
//!   @endstring
//!
//!   LDAP (RFC2307)-style. Don't use these if you can avoid it,
//!   since they are suspectible to attacks. In particular avoid
//!   the unsalted variants at all costs:
//!   @string
//!     @value "ssha"
//!     @value "{ssha}"
//!       @[SHA1.hash()] with 96 bits of salt appended to the password.
//!
//!     @value "smd5"
//!     @value "{smd5}"
//!       @[MD5.hash()] with 96 bits of salt appended to the password.
//!
//!     @value "sha"
//!     @value "{sha}"
//!       @[SHA1.hash()] without any salt.
//!
//!     @value "md5"
//!     @value "{md5}"
//!       @[MD5.hash()] without any salt.
//!   @endstring
//!
//! @param rounds
//!   The number of rounds to use in parameterized schemes. If not
//!   specified the scheme specific default will be used.
//!
//! @returns
//!   Returns a string suitable for @[verify()]. This means that
//!   the hashes will be prepended with the suitable markers.
//!
//! @note
//!   Note that the availability of @[SHA512] depends on the version
//!   of @[Nettle] that Pike has been compiled with.
//!
//! @note
//!   This function was added in Pike 7.8.755.
//!
//! @seealso
//!   @[verify()], @[predef::crypt()], @[Nettle.crypt_md5()],
//!   @[Nettle.Hash()->crypt_hash()]
string(7bit) hash(string(8bit) password, string|void scheme, int|void rounds)
{
  function(string(8bit), string(7bit), int:string(8bit)) crypt_hash;
  int salt_size = 16;
  int default_rounds = 5000;

  string(7bit) render_crypt_hash(string(7bit) scheme, string(7bit) salt,
                                 string(8bit) hash, int rounds)
  {
    if (rounds != default_rounds) {
      salt = "rounds=" + rounds + "$" + salt;
    }

    // We claim this to be a string(7bit) string, even though we add
    // the string(0..256). It will however only be called with the
    // already base64 encoded hashes.
    return [string(7bit)]sprintf("$%s$%s$%s", scheme, salt, hash);
  };

  string(7bit) render_ldap_hash(string(8bit) scheme, string(7bit) salt,
                                string(8bit) hash, int rounds)
  {
    if (scheme[0] != '{') scheme = "{" + scheme + "}";
    return [string(7bit)]upper_case(scheme) + MIME.encode_base64(hash + salt);
  };

  function(string(7bit), string(7bit), string(8bit), int:string(7bit)) render_hash = render_crypt_hash;

  switch(lower_case(scheme)) {
  case "crypt":
  case "{crypt}":
  case UNDEFINED:
    // FALL_THROUGH
#if constant(Crypto.SHA512)
  case "6":
  case "$6$":
    crypt_hash = Crypto.SHA512.crypt_hash;
    scheme = "6";
    break;
#endif
  case "5":
  case "$5$":
    crypt_hash = Crypto.SHA256.crypt_hash;
    scheme = "5";
    break;
  case "1":
  case "$1$":
    crypt_hash = Crypto.MD5.crypt_hash;
    salt_size = 8;
    rounds = 1000;		// Currently only 1000 rounds is supported.
    default_rounds = 1000;
    scheme = "1";
    break;
  case "":
    return crypt(password);

  case "sha":
  case "{sha}":
    salt_size = 0;
    // FALL_THROUGH
  case "ssha":
  case "{ssha}":
    crypt_hash = lambda(string(8bit) passwd, string(7bit) salt, int rounds) {
		   return Crypto.SHA1.hash(passwd + salt);
		 };
    render_hash = render_ldap_hash;
    break;

  case "md5":
  case "{md5}":
    salt_size = 0;
    // FALL_THROUGH
  case "smd5":
  case "{smd5}":
    crypt_hash = lambda(string(8bit) passwd, string(8bit) salt, int rounds) {
		   return Crypto.MD5.hash(passwd + salt);
		 };
    render_hash = render_ldap_hash;
    break;

  default:
    error("Unsupported hashing scheme: %O\n", scheme);
  }

  if (!rounds) rounds = default_rounds;

  // NB: The salt must be printable.
  string(7bit) salt =
    MIME.encode_base64(Crypto.Random.random_string(salt_size))[..salt_size-1];

  string(8bit) hash = crypt_hash(password, salt, rounds);

  return render_hash([string(7bit)]scheme, salt, hash, rounds);
}

