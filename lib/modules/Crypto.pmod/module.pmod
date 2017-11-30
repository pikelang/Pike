#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.Hash)

//! Various cryptographic classes and functions.
//!
//! @dl
//!   @item Hash modules
//!     These are based on the @[Nettle.Hash] API.
//!     Examples include @[MD5], @[SHA1], @[SHA256] and @[SHA3_256].
//!
//!   @item Cipher modules
//!     These are based on the @[Nettle.Cipher] API.
//!     Examples include @[AES], @[Arcfour], @[DES], @[DES3], @[CAMELLIA].
//!
//!     The @[Substitution] program is compatible with @[Cipher.State].
//!
//!     Also conforming to the API are several helper modules such as
//!     @[Buffer], @[predef::Nettle.BlockCipher.CBC],
//!     @[predef::Nettle.BlockCipher16.GCM] and @[Pipe].
//!
//!   @item Message Authentication Code modules (MACs)
//!     @[MAC] algorithms are provided as sub-modules to their corresponding
//!     @[Hash] or @[Cipher] module.
//!     Examples include @[SHA1.HMAC] and @[AES.UMAC32].
//!
//!   @item Authenticated Encryption with Associated Data modules (AEADs)
//!     @[AEAD]s combine ciphers with authentication codes, and may optionally
//!     also take into account some associated data that is provided out of band.
//!     This API is compatible with both @[Cipher] and @[Hash].
//!     AEADs are provided as sub-modules to their corresponding ciphers.
//!     Examples include @[AES.CCM], @[AES.GCM] and @[CAMELLIA.EAX].
//! @enddl
//!
//! As the cryptographic services offered from this module aren't
//! necessarily used for security applications, none of the strings
//! inputted or outputted are marked as secure. That is up to the
//! caller.
//!
//! @note
//!   Most of the APIs in this module work on 8 bit binary strings unless
//!   otherwise noted.
//!   For conversions to and from hexadecimal notation @[String.string2hex()]
//!   and @[String.hex2string()] may be of interest.
//!
//! @note
//!   This module is only available if Pike has been compiled with
//!   @[Nettle] enabled (this is the default).

constant HashState = Nettle.Hash.State;
constant siphash24 = Builtin.siphash24;

//! Abstract class for AE algorithms.
class AE {
  inherit __builtin.Nettle.AE;
}

//! Abstract class for AEAD algorithms.
class AEAD {
  inherit Nettle.AEAD;
}

//! Abstract class for hash algorithms. Contains some tools useful
//! for all hashes.
class Hash {
  inherit Nettle.Hash;
}

//! Abstract class for Message Authentication Code (MAC) algorithms.
//! Contains some tools useful for all MACs.
class MAC {
  inherit Nettle.MAC;
}

//! Abstract class for signature algorithms. Contains some tools useful
//! for all signatures.
class Sign {
  //! Returns the printable name of the signing algorithm.
  string(7bit) name();

  //!
  class State {
    inherit __builtin.Nettle.Sign;
  }

  //! Calling `() will return a @[State] object.
  protected State `()() { return State(); }
}

//! Hashes a @[password] together with a @[salt] with the
//! crypt_md5 algorithm and returns the result.
//!
//! @seealso
//!   @[verify_crypt_md5]
string(8bit) make_crypt_md5(string(8bit) password,
                            void|string(8bit) salt)
{
  if(salt)
    sscanf(salt, "%s$", salt);
  else
    salt = ([function(string(8bit):string(7bit))]MIME["encode_base64"])
      (.Random.random_string(6));

  return "$1$"+salt+"$"+Nettle.crypt_md5(password, salt);
}

//! Verifies the @[password] against the crypt_md5 hash.
//! @throws
//!   May throw an exception if the hash value is bad.
//! @seealso
//!   @[make_crypt_md5]
bool verify_crypt_md5(string(8bit) password, string(7bit) hash)
{
  string(7bit) salt;
  if( sscanf(hash, "$1$%s$%s", salt, hash)!=2 )
    error("Error in hash.\n");
  return Nettle.crypt_md5(password, salt) == hash;
}

constant CipherState = Nettle.Cipher.State;

//! Abstract class for crypto algorithms. Contains some tools useful
//! for all ciphers.
//!
//! @note
//!   Typically only inherited directly by stream ciphers.
//!
//! @note
//!   It is however convenient for typing as it contains the minimum
//!   base level API for a cipher.
//!
//! @seealso
//!   @[BufferedCipher], @[BlockCipher], @[BlockCipher16]
class Cipher
{
  inherit Nettle.Cipher;
}

//! Abstract class for block cipher meta algorithms.
//!
//! Contains the @[Buffer] submodule.
class BufferedCipher
{
  inherit Nettle.BufferedCipher;
}

//! Abstract class for block cipher algorithms. Contains some tools useful
//! for all block ciphers.
//!
//! Contains the @[CBC] submodule.
class BlockCipher
{
  inherit Nettle.BlockCipher;
}

//! Abstract class for block cipher algorithms with a 16 byte block size.
//! Contains some tools useful for all such block ciphers.
//!
//! Contains the @[GCM] submodule.
class BlockCipher16
{
  inherit Nettle.BlockCipher16;
}

//! @decl string(8bit) rot13(string(8bit) data)
//! Convenience function that accesses the crypt function of a
//! substitution object keyed to perform standard ROT13 (de)ciphering.

constant PAD_SSL       = Nettle.PAD_SSL;
constant PAD_ISO_10126 = Nettle.PAD_ISO_10126;
constant PAD_ANSI_X923 = Nettle.PAD_ANSI_X923;
constant PAD_PKCS7     = Nettle.PAD_PKCS7;
constant PAD_ZERO      = Nettle.PAD_ZERO;
constant PAD_TLS       = Nettle.PAD_TLS;


//! Typedef for a generic hashing function that takes a string as first
//! argument, an optional @tt{bool@} (returns a binary hash if true) and
//! returns a hashed string.
typedef function(string(8bit),void|bool:string) hash_func_t;

//! Typedef for a generic HMAC hash function that takes data and a secret as
//! arguments and returns a hashed string or a @[hash_func_t] function.
typedef function(string(8bit),void|string(8bit):string|hash_func_t)
  hash_hmac_func_t;

//! Functor returning a new function which will hash its input with the
//! algorithm in @[hash]. Per default the function returns a hexadecimal string,
//! but if @[raw] is @tt{true@} the raw hash is returned.
//!
//! This is the low level version of @[Crypto.hash()].
//!
//! @seealso
//!  @[Crypto.hash()], @[hash_hmac()], @[md5()], @[sha1()], @[sha256()]
hash_func_t make_hash_func(Crypto.Hash hash, void|bool raw) {
  return lambda (string(8bit) in, void|bool _raw) {
    string s = hash->hash(in);
    bool do_raw = zero_type(_raw) ? raw : _raw;
    return do_raw ? s : String.string2hex(s);
  };
}


//! Functor returning a new function which will HMAC hash its data and secret
//! with the algorithm in @[hash]. Per default the function returns a
//! hexadecimal string, but if @[raw] is @tt{true@} the raw hash is returned.
//!
//! This is the low level version of @[hash_hmac()].
//!
//! @seealso
//!  @[hash_hmac()], @[Crypto.hash()], @[hmac_md5()], @[hmac_sha1()],
//!  @[hmac_sha256()]
hash_hmac_func_t make_hash_hmac_func(Crypto.Hash hash, void|bool raw) {
  return lambda (string(8bit) secret, void|string(8bit) data) {
    if (!data) {
      Crypto.MAC.State f = Crypto.HMAC(hash)(secret);

      return lambda (string(8bit) data, void|bool _raw) {
        string s = f(data);
        bool do_raw = zero_type(_raw) ? raw : _raw;
        return do_raw ? s : String.string2hex(s);
      };
    }

    string s = Crypto.HMAC(hash)(secret)(data);
    return raw ? s : String.string2hex(s);
  };
}


//! The @[hash_hmac()] function can be used in three different ways:
//!
//! @ol
//!  @item
//!   The first will return a hash of @tt{secret@} and @tt{data@} using the
//!   @[Crypto.Hash] algorithm passed as first argument.
//!
//!   @code
//!   string hex_hash = Crypto.hash_hmac(Crypto.SHA256, "secret", "data");
//! string bin_hash = Crypto.hash_hmac(Crypto.SHA256, "secret", "data", true);
//!   @endcode
//!
//!  @item
//!   The second will return a HMAC hashing function using the algorithm in
//!   the @[Crypto.Hash] given as first argument, and the @[secret] given as
//!   second argument as the hashing secret.
//!
//!   @code
//!   function my_hasher = Crypto.hash_hmac(Crypto.SHA256, "secret");
//! string hex_hash = my_hasher("data");
//! string bin_hash = my_hasher("data", true);
//!   @endcode
//!
//!  @item
//!   And the third will return a function taking the secret as first argument
//!   and the data as second and hash those using the algorithm in the
//!   @[Crypto.Hash] given as first argument.
//!
//!   @code
//!   function my_hasher = Crypto.hash_hmac(Crypto.SHA256, true);
//! string bin_hash1 = my_hasher("secret", "data");
//! string bin_hash2 = my_hasher("other-secret", "data");
//!   @endcode
//! @endol
//!
//! By default the returned function will return a hexadecimal string, but if
//! the last argument, @[raw], is given the binary hash will be returned.
//!
//! @seealso
//!  @[hmac_md5()], @[hmac_sha1()], @[hmac_sha256()], @[hash()], @[md5()],
//!  @[sha1()], @[sha256]
string hash_hmac(Crypto.Hash hash, string(8bit) secret, string(8bit) data,
                 void|bool raw)
{
  return (string)make_hash_hmac_func(hash, raw)(secret, data);
}
variant hash_func_t hash_hmac(Crypto.Hash hash, string(8bit) secret,
                              void|bool raw)
{
  return (hash_func_t)(make_hash_hmac_func(hash, raw)(secret));
}
variant hash_hmac_func_t hash_hmac(Crypto.Hash hash, void|bool raw)
{
  return make_hash_hmac_func(hash, raw);
}


//! Creates eiter a hash of @[data] using the hashing algorithm in @[hash] or
//! a hashing function using the hashing algorithm in @[hash].
//!
//! If @[raw] is given the binary hash will be returned, otherwise a
//! hexadecimal hash is returned.
//!
//! @example
//! @code
//! function sha256_raw = Crypto.hash(Crypto.SHA256, true);
//! string bin_hash = sha256_raw("My data");
//!
//! function sha1 = Crypto.hash(Crypto.SHA1);
//! string hex_hash = sha1("My data");
//! @endcode
string hash(Crypto.Hash hash, string(8bit) data, void|bool raw)
{
  return make_hash_func(hash, raw)(data);
}
variant hash_func_t hash(Crypto.Hash hash, void|bool raw)
{
  return make_hash_func(hash, raw);
}


//! @decl string(8bit) hmac_md5(string(8bit) secret, string(8bit) data)
//! @decl string(8bit) hmac_sha1(string(8bit) secret, string(8bit) data)
//! @decl string(8bit) hmac_sha256(string(8bit) secret, string(8bit) data)
//!
//! @[HMAC] hashing functions using the algorithms in @[Crypto.MD5],
//! @[Crypto.SHA1] and @[Crypto.SHA256].
//!
//! The returned strings are in hexadecimal format. If you want the binary
//! string use @[hash_hmac()].
//!
//! @seealso
//!  @[hash_hmac()], @[hash()], @[md5()], @[sha1()], @[sha256]
//-- NO PIKEDOC HERE PLEASE
hash_hmac_func_t hmac_md5 = hash_hmac(Crypto.MD5);
hash_hmac_func_t hmac_sha1 = hash_hmac(Crypto.SHA1);
hash_hmac_func_t hmac_sha256 = hash_hmac(Crypto.SHA256);


//! @decl string(8bit) md5(string(8bit) data, void|bool raw)
//! @decl string(8bit) sha1(string(8bit) data, void|bool raw)
//! @decl string(8bit) sha256(string(8bit) data, void|bool raw)
//!
//! Hashing function using @[Crypto.MD5], @[Crypto.SHA1] and @[Crypto.SHA256]
//! as hashing algorithms.
//!
//! If @[raw] is @tt{true@} the functions returns the binary string, otherwise
//! a hexadecimal string.
//!
//! @seealso
//!  @[hash()], @[hash_hmac()], @[hmac_md5()], @[hmac_sha1()], @[hmac_sha256()]
//-- NO PIKEDOC HERE PLEASE
hash_func_t md5 = hash(Crypto.MD5);
hash_func_t sha1 = hash(Crypto.SHA1);
hash_func_t sha256 = hash(Crypto.SHA256);
