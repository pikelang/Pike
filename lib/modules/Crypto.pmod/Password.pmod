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
//! LDAP-style (@rfc{2307@}) hashes:
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
//!   @value "$3$$XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX"
//!     This is interpreted as the NT LANMANAGER (NTLM) password
//!     hash. It is a hex representation of MD4 of the password.
//!
//!   @value "$1$SSSSSSSS$XXXXXXXXXXXXXXXXXXXXXX"
//!     The string is interpreted according to the GNU libc2 extension
//!     of @expr{crypt(3C)@} where @expr{SSSSSSSS@} is up to 8 chars of
//!     salt and the @expr{XXX@} string is an @[MD5]-based hash created
//!     from the password and the salt. Source: GNU libc
//!     @url{http://www.gnu.org/software/libtool/manual/libc/crypt.html@}.
//!
//!   @value "$sha1$RRRRR$SSSSSSSS$XXXXXXXXXXXXXXXXXXXX"
//!     The string is interpreted as a NetBSD-style @[SHA1.HMAC.crypt_hash()]
//!     (aka @tt{crypt_sha1(3C)@}),
//!     where @expr{RRRRR@} is the number of rounds (default 480000),
//!     @expr{SSSSSSSS@} is a @[MIME.crypt64()] encoded salt. and the
//!     @expr{XXX@} string is an @[SHA1.HMAC]-based hash created from
//!     the password and the salt.
//!
//!   @value "$P$RSSSSSSSSXXXXXXXXXXXXXXXXXXXXXX"
//!     The string is interpreted as a PHPass' Portable Hash password hash,
//!     where @expr{R@} is an encoding of the 2-logarithm of the number of
//!     rounds, @expr{SSSSSSSS@} is a salt of 8 characters, and
//!     @expr{XXX@} is similarily the @[MIME.encode_crypt64] of running
//!     @[MD5.hash()] repeatedly on the password and the salt.
//!
//!   @value "$H$RSSSSSSSS.XXXXXXXXXXXXXXXXXXXXXX"
//!     Same as @expr{"$P$"@} above. Used by phpBB3.
//!
//!   @value "U$P$RSSSSSSSSXXXXXXXXXXXXXXXXXXXXXX"
//!     This is handled as a Drupal upgraded PHPass Portable Hash password.
//!     The password is run once through @[MD5.hash()], and then passed
//!     along to the @expr{"$P$"@}-handler above.
//!
//!   @value "$Q$RSSSSSSSSXXXXXXXXXXXXXXXXXXXXXX"
//!     The string is interpreted as a PHPass' Portable Hash password hash,
//!     where the base hashing alorithm has been switched to @[SHA1].
//!     This method is apparently used by some versions of Escher CMS.
//!
//!   @value "$S$RSSSSSSSSXXXXXXXXXXXXXXXXXXXXXX"
//!     The string is interpreted as a PHPass' Portable Hash password hash,
//!     where the base hashing alorithm has been switched to @[SHA256].
//!     This method is apparently used by some versions of Drupal.
//!
//!   @value "XXXXXXXXXXXXX"
//!     The @expr{XXX@} string (which doesn't begin with @expr{"{"@}) is
//!     taken to be a password hashed using the classic unix
//!     @expr{crypt(3C)@} function. If the string contains only chars
//!     from the set @expr{[a-zA-Z0-9./]@} it uses DES and the first two
//!     characters as salt, but other alternatives may be possible
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
int verify(string(8bit) password, string(7bit) hash)
{
  if (hash == "") return 1;

  string(8bit) passwd = password;
  password = "CENSORED";

  // Compatibility with Drupal upgraded passwords.
  if (has_prefix(hash, "U$P")) {
    password = Crypto.MD5.hash(password);
    hash = hash[1..];
  }

  // Detect the password hashing scheme.
  // First check for an LDAP-style marker.
  string(7bit) scheme = "crypt";
  sscanf(hash, "{%s}%s", scheme, hash);
  // NB: RFC2307 proscribes lower case schemes, while
  //     in practise they are usually in upper case.
  switch(lower_case(scheme)) {
  case "md5":	// RFC 2307
  case "smd5":
    string(8bit) bin_hash = MIME.decode_base64(hash);
    passwd += bin_hash[16..];
    bin_hash = bin_hash[..15];
    return Crypto.MD5.hash(passwd) == bin_hash;

  case "sha":	// RFC 2307
  case "ssha":
    // SHA1 and Salted SHA1.
    bin_hash = MIME.decode_base64(hash);
    passwd += bin_hash[20..];
    bin_hash = bin_hash[..19];
    return Crypto.SHA1.hash(passwd) == bin_hash;

  case "crypt":	// RFC 2307
    // First try the operating systems crypt(3C),
    // since it might support more schemes than we do.
    catch {
      if ((hash == "") || crypt(passwd, hash)) return 1;
    };
    if (hash[0] != '$') {
      if (hash[0] == '_') {
	// FIXME: BSDI-style crypt(3C).
      }
      return 0;
    }

    string(7bit) fullhash = hash;
    // Then try our implementations.
    if (!sscanf(hash, "$%s$%s", scheme, hash)) return 0;
    sscanf(hash, "%s$%s", string(7bit) salt, hash);
    int rounds = UNDEFINED;
    switch(scheme) {
    case "1":	// crypt_md5
      return Nettle.crypt_md5(passwd, salt) == hash;

#if constant(Nettle.bcrypt_hash)
    case "2":	// Blowfish (obsolete)
    case "2a":	// Blowfish (possibly weak)
    case "2b":	// Blowfish (long password bug fixed)
    case "2x":	// Blowfish (weak)
    case "2y":	// Blowfish (stronger)
      return Nettle.bcrypt_verify(passwd, fullhash);
#endif

    case "nt":
    case "3":	// MD4 NT LANMANAGER (FreeBSD)
      return this::hash(passwd, scheme)[sizeof(scheme) + 3..] == hash;

      // cf http://www.akkadia.org/drepper/SHA-crypt.txt
    case "5":	// SHA-256
      if (salt && has_prefix(salt, "rounds=")) {
	sscanf(salt, "rounds=%d", rounds);
	sscanf(hash, "%s$%s", salt, hash);
      }
      return Crypto.SHA256.crypt_hash(passwd, salt, rounds) == hash;

#if constant(Crypto.SHA512)
    case "6":	// SHA-512
      if (salt && has_prefix(salt, "rounds=")) {
	sscanf(salt, "rounds=%d", rounds);
	sscanf(hash, "%s$%s", salt, hash);
      }
      return Crypto.SHA512.crypt_hash(passwd, salt, rounds) == hash;
#endif

    case "pbkdf2":		// PBKDF2 with SHA1
      rounds = (int)salt;
      sscanf(hash, "%s$%s", salt, hash);
      return Crypto.SHA1.crypt_pbkdf2(passwd, salt, rounds) == hash;

    case "pbkdf2-sha256":	// PBKDF2 with SHA256
      rounds = (int)salt;
      sscanf(hash, "%s$%s", salt, hash);
      return Crypto.SHA256.crypt_pbkdf2(passwd, salt, rounds) == hash;

#if constant(Crypto.SHA512)
    case "pbkdf2-sha512":	// PBKDF2 with SHA512
      rounds = (int)salt;
      sscanf(hash, "%s$%s", salt, hash);
      return Crypto.SHA512.crypt_pbkdf2(passwd, salt, rounds) == hash;
#endif

    case "H": case "P":	// PHPass Portable Hash.
      salt = hash[..8];
      hash = hash[9..];
      return Crypto.MD5.crypt_php(passwd, salt) == hash;
      break;

    case "Q":	// PHPass Portable Hash SHA1.
      salt = hash[..8];
      hash = hash[9..];
      return Crypto.SHA1.crypt_php(passwd, salt) == hash;
      break;

#if constant(Crypto.SHA512)
    case "S":	// PHPass Portable Hash SHA512.
      salt = hash[..8];
      hash = hash[9..];
      return Crypto.SHA512.crypt_php(passwd, salt) == hash;
      break;
#endif

    case "sha1":	// SHA1-HMAC
      rounds = (int)salt;
      sscanf(hash, "%s$%s", salt, hash);
      return Crypto.SHA1.HMAC.crypt_hash(passwd, salt, rounds) == hash;
      break;
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
//!     @value "3"
//!     @value "NT"
//!       The NTLM MD4 hash.
//!
//!     @value "2"
//!     @value "2a"
//!     @value "2b"
//!     @value "2x"
//!     @value "2y"
//!     @value "$2$"
//!     @value "$2a$"
//!     @value "$2b$"
//!     @value "$2x$"
//!     @value "$2y$"
//!       @[Nettle.bcrypt()] with 128 bits of salt and a default
//!       of @expr{1024@} rounds.
//!
//!     @value "1"
//!     @value "$1$"
//!       @[MD5.crypt_hash()] with 48 bits of salt and @expr{1000@} rounds.
//!
//!     @value "sha1"
//!       @[SHA1.HMAC.crypt_hash()] with 48 bits of salt and a default
//!       of @expr{480000@} rounds.
//!
//!     @value "P"
//!     @value "$P$"
//!     @value "H"
//!     @value "$H$"
//!       @[MD5.crypt_php()] with 48 bits of salt and a default of
//!       @expr{1<<19@} rounds. The specified number of rounds will
//!       be rounded up to the closest power of @expr{2@}.
//!
//!     @value "U$P$"
//!       Same as @expr{"$P$"@}, the supplied @[password] is assumed to
//!       have already been passed through @[MD5.hash()] once. Typically
//!       used to upgrade unsalted @[MD5]-password databases.
//!
//!     @value "Q"
//!     @value "$Q$"
//!       Same as @expr{"$P$"@}, but with @[SHA1.crypt_php()].
//!
//!     @value "S"
//!     @value "$S$"
//!       Same as @expr{"$S$"@}, but with @[SHA512.crypt_php()].
//!
//!     @value "pbkdf2"
//!     @value "$pbkdf2$"
//!       @[SHA1.pbkdf2()].
//!
//!     @value "pbkdf2-sha256"
//!     @value "$pbkdf2-sha256$"
//!       @[SHA256.pbkdf2()].
//!
//!     @value "pbkdf2-sha512"
//!     @value "$pbkdf2-sha512$"
//!       @[SHA512.pbkdf2()].
//!
//!     @value ""
//!       @[predef::crypt()] with 12 bits of salt.
//!
//!   @endstring
//!
//!   LDAP (@rfc{2307@})-style. Don't use these if you can avoid it,
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
string(7bit) hash(string(8bit) password, string(7bit)|void scheme,
		  int(0..)|void rounds)
{
  function(string(8bit), string(7bit), int(0..):string(7bit)) crypt_hash;
  int(0..) salt_size = 16;
  int(0..) default_rounds = 5000;

  string(7bit) render_crypt_hash(string(7bit) scheme, string(7bit) salt,
                                 string(7bit) hash, int rounds)
  {
    if (rounds != default_rounds) {
      salt = "rounds=" + rounds + "$" + salt;
    }
    return sprintf("$%s$%s$%s", scheme, salt, hash);
  };

  string(7bit) render_old_crypt_hash(string(7bit) scheme, string(7bit) salt,
				     string(7bit) hash, int rounds)
  {
    return sprintf("$%s$%d$%s$%s", scheme, rounds, salt, hash);
  };

  string(7bit) render_php_crypt_hash(string(7bit) scheme, string(7bit) salt,
				     string(7bit) hash, int rounds)
  {
    if (!has_value(scheme, '$')) scheme = "$" + scheme + "$";
    int(0..) exp2 = 7;
    while (1 << exp2 < rounds && ++exp2 < 30);
    return sprintf("%s%c%s%s",
		   scheme, "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[exp2],
		   salt, hash);
  };

  string(7bit) render_ldap_hash(string(7bit) scheme, string(7bit) salt,
                                string(7bit) hash, int rounds)
  {
    if (scheme[0] != '{') scheme = "{" + scheme + "}";
    return [string(7bit)](upper_case(scheme) + hash);
  };

  function(string(7bit), string(7bit), string(7bit), int(0..):string(7bit))
    render_hash = render_crypt_hash;

  string schemeprefix = scheme;

  if (schemeprefix && schemeprefix[0] == '$')
    sscanf(schemeprefix, "$%s$", schemeprefix);

  switch(lower_case(schemeprefix)) {
  case "crypt":
  case "{crypt}":
  case UNDEFINED:
    // FALL_THROUGH
#if constant(Crypto.SHA512)
  case "6":
    crypt_hash = Crypto.SHA512.crypt_hash;
    scheme = "6";
    break;
#endif
  case "5":
    crypt_hash = Crypto.SHA256.crypt_hash;
    scheme = "5";
    break;
#if constant(Nettle.bcrypt_hash)
  case "2":	// Blowfish (obsolete)
  case "2a":	// Blowfish (possibly weak)
  case "2b":	// Blowfish (long password bug fixed)
  case "2x":	// Blowfish (weak)
  case "2y":	// Blowfish (stronger)
  {
    string(8bit) salt;
    int exp2 = -1;
    if (rounds) {
      int(0..) exp2;
      for (exp2 = 0; 1 << exp2 < rounds && ++exp2 < 31; );
      rounds = exp2;
    }
    if (sizeof(scheme) < 1 + 2 + 1 + 2 + 1 + 22)
      salt = random_string(16);
    return Nettle.bcrypt_hash(password, scheme, salt, rounds);
  }
#endif
  case "1":
    crypt_hash = Crypto.MD5.crypt_hash;
    salt_size = 8;
    rounds = 1000;		// Currently only 1000 rounds is supported.
    default_rounds = 1000;
    scheme = "1";
    break;
  case "":
    return crypt(password);

  case "nt":
    scheme = "NT";
  case "3":
    password = [string(8bit)](reverse((string_to_unicode(password)/2)[*])*"");
    return "$"+scheme+"$$"+String.string2hex(Crypto.MD4.hash(password));

  case "sha":
  case "{sha}":
    salt_size = 0;
    // FALL_THROUGH
  case "ssha":
  case "{ssha}":
    crypt_hash = lambda(string(8bit) passwd, string(7bit) salt, int rounds) {
		   return MIME.encode_base64(Crypto.SHA1.hash(passwd + salt) +
					     salt);
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
		   return MIME.encode_base64(Crypto.MD5.hash(passwd + salt) +
					     salt);
		 };
    render_hash = render_ldap_hash;
    break;

  case "sha1":
    // NetBSD-style crypt_sha1().
    crypt_hash = Crypto.SHA1.HMAC.crypt_hash;
    render_hash = render_old_crypt_hash;
    // Defaults taken from PassLib.
    salt_size = 8;
    rounds = 480000;
    break;

  case "pbkdf2":
    crypt_hash = Crypto.SHA1.crypt_pbkdf2;
    render_hash = render_old_crypt_hash;
    // Defaults taken from PassLib.
    salt_size = 22;		// 16 bytes after base64-encoding.
    default_rounds = 29000;	// NB: The Passlib example defaults to 6400.
    scheme = "pbkdf2";
    break;

  case "pbkdf2-sha256":
    crypt_hash = Crypto.SHA256.crypt_pbkdf2;
    render_hash = render_old_crypt_hash;
    // Defaults taken from PassLib.
    salt_size = 22;		// 16 bytes after base64-encoding.
    default_rounds = 29000;	// NB: The Passlib example defaults to 6400.
    scheme = "pbkdf2-sha256";
    break;

#if constant(Crypto.SHA512)
  case "pbkdf2-sha512":
    crypt_hash = Crypto.SHA512.crypt_pbkdf2;
    render_hash = render_old_crypt_hash;
    // Defaults taken from Passlib.
    salt_size = 22;		// 16 bytes after base64-encoding.
    default_rounds = 29000;	// NB: The Passlib example defaults to 6400.
    scheme = "pbkdf2-sha512";
    break;
#endif

  case "P":
  case "U$P$":
  case "H":
    crypt_hash = Crypto.MD5.crypt_php;
    render_hash = render_php_crypt_hash;
    salt_size = 8;
    default_rounds = 1<<19;
    break;

  case "Q":
    crypt_hash = Crypto.SHA1.crypt_php;
    render_hash = render_php_crypt_hash;
    salt_size = 8;
    default_rounds = 1<<19;
    break;

#if constant(Crypto.SHA512)
  case "S":
    crypt_hash = Crypto.SHA512.crypt_php;
    render_hash = render_php_crypt_hash;
    salt_size = 8;
    default_rounds = 1<<19;
    break;
#endif

  default:
    error("Unsupported hashing scheme: %O\n", scheme);
  }

  if (!rounds) rounds = default_rounds;

  // NB: The salt must be printable.
  string(7bit) salt =
    [string(7bit)]replace(MIME.encode_base64(random_string(salt_size))[..salt_size-1], "+", ".");

  string(7bit) hash = crypt_hash(password, salt, rounds);

  return render_hash([string(7bit)]scheme, salt, hash, rounds);
}
