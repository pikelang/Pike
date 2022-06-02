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
//! input or output are marked as secure. That is up to the
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
    sscanf([string]salt, "%s$", salt);
  else
    salt = ([function(string(8bit):string(7bit))]MIME["encode_base64"])
      (.Random.random_string(6));

  return "$1$" + [string]salt + "$" + Nettle.crypt_md5(password, [string]salt);
}

//! Verifies the @[password] against the crypt_md5 hash.
//! @throws
//!   May throw an exception if the hash value is bad.
//! @seealso
//!   @[make_crypt_md5]
bool verify_crypt_md5(string(8bit) password, string(7bit) hash)
{
  string(7bit) salt = "";
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
