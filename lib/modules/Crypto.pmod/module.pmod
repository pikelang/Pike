#pike __REAL_VERSION__
#pragma strict_types

//! Various cryptographic classes and functions.
//!
//! @b{Hash functions@}
//! These are based on the @[Hash] API; @[MD2], @[MD4], @[MD5],
//! @[SHA1], @[SHA256].
//!
//! @b{Stream cipher functions@}
//! These are based on the @[Cipher] API; @[AES], @[Arcfour],
//! @[Blowfish], @[CAST], @[DES], @[DES3], @[IDEA], @[Serpent],
//! @[Twofish]. The @[Substitution] program is compatible with the
//! CipherState. Also conforming to the API are the helper programs
//! @[Buffer], @[CBC], @[GCM] and @[Pipe].
//!
//! As the cryptographic services offered from this module isn't
//! necessarily used for security applications, none of the strings
//! inputted or outputted are marked as secure. This is up to the
//! caller.
//!
//! @note
//!   This module is only available if Pike has been compiled with
//!   @[Nettle] enabled (this is the default).

#if constant(Nettle.Hash)

constant HashState = Nettle.Hash.State;

//! Abstract class for hash algorithms. Contains some tools useful
//! for all hashes.
class Hash {
  inherit Nettle.Hash;
}

//! Hashes a @[password] together with a @[salt] with the
//! crypt_md5 algorithm and returns the result.
//!
//! @seealso
//!   @[verify_crypt_md5]
string(0..255) make_crypt_md5(string(0..255) password,
			      void|string(0..255) salt)
{
  if(salt)
    sscanf(salt, "%s$", salt);
  else
    salt = ([function(string(0..255):string(0..255))]MIME["encode_base64"])
      (.Random.random_string(6));

  return "$1$"+salt+"$"+Nettle.crypt_md5(password, salt);
}

//! Verifies the @[password] against the crypt_md5 hash.
//! @throws
//!   May throw an exception if the hash value is bad.
//! @seealso
//!   @[make_crypt_md5]
int(0..1) verify_crypt_md5(string(0..255) password, string(0..127) hash)
{
  string(0..127) salt;
  if( sscanf(hash, "$1$%s$%s", salt, hash)!=2 )
    error("Error in hash.\n");
  return Nettle.crypt_md5(password, salt) == hash;
}

constant CipherState = Nettle.Cipher.State;

//! Abstract class for crypto algorithms. Contains some tools useful
//! for all ciphers.
class Cipher
{
  inherit Nettle.Cipher;
}

//! Implementation of the cipher block chaining mode (CBC). Works as
//! a wrapper for the cipher algorithm put in create.
//!
//! @seealso
//!   @[GCM]
class CBC {
  inherit Nettle.CBC;
}

//! Acts as a buffer so that data can be fed to a cipher in blocks
//! that don't correspond to cipher block sizes.
//!
//! @example
//!   class Encrypter
//!   {
//!     protected Crypto.Buffer buffer;
//!
//!     void create(string key)
//!     {
//!       buffer = Crypto.Buffer(Crypto.CBC(Crypto.AES));
//!       buffer->set_encrypt_key(key);
//!     }
//!
//!     string feed(string data)
//!     {
//!       return buffer->crypt(data);
//!     }
//!
//!     string drain()
//!     {
//!       return buffer->pad(Crypto.PAD_PKCS7);
//!     }
//!   }
class Buffer {
  inherit Nettle.Proxy;
}

//! @decl string(0..255) rot13(string(0..255) data)
//! Convenience function that accesses the crypt function of a
//! substitution object keyed to perform standard ROT13 (de)ciphering.

function(string(0..255):string(0..255)) rot13 =
  Crypto.Substitution()->set_rot_key()->crypt;

constant PAD_SSL = 0;
constant PAD_ISO_10126 = 1;
constant PAD_ANSI_X923 = 2;
constant PAD_PKCS7 = 3;
constant PAD_ZERO = 4;

#else
constant this_program_does_not_exist=1;
#endif /* constant(Nettle.Hash) */
