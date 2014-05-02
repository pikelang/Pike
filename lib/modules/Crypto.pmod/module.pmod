#pike __REAL_VERSION__
#pragma strict_types
#require constant(Nettle.Hash)

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
//! CipherState. Also conforming to the API are the helper modules
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

constant HashState = Nettle.Hash.State;

//! Abstract class for AEAD algorithms.
class AEAD {
  inherit Nettle.AEAD;
}

//! Abstract class for hash algorithms. Contains some tools useful
//! for all hashes.
class Hash {
  inherit Nettle.Hash;
}

//! Abstract class for MAC algorithms.
class MAC {
  inherit Nettle.MAC;
}

//! Abstract class for signature algorithms. Contains some tools useful
//! for all signatures.
class Sign {
  inherit __builtin.Nettle.Sign;
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

function(string(8bit):string(8bit)) rot13 =
  Crypto.Substitution()->set_rot_key()->crypt;

constant PAD_SSL       = Nettle.PAD_SSL;
constant PAD_ISO_10126 = Nettle.PAD_ISO_10126;
constant PAD_ANSI_X923 = Nettle.PAD_ANSI_X923;
constant PAD_PKCS7     = Nettle.PAD_PKCS7;
constant PAD_ZERO      = Nettle.PAD_ZERO;
constant PAD_TLS       = Nettle.PAD_TLS;

#pragma no_deprecation_warnings

// Compatibility stuff.

protected constant compat_required_methods = ({
  "block_size",
  "key_size",
  "set_encrypt_key",
  "set_decrypt_key",
  "crypt",
});

protected void compat_assert_is_crypto_object(CipherState obj)
{
  foreach(compat_required_methods, string method) {
    if (!obj[method]) error("Object is missing identifier \"%s\"\n", method);
  }
}

//! @ignore
protected class CompatProxy
{
  inherit BlockCipher;

  class _CBC {
    inherit ::this_program;

    class State
    {
      inherit ::this_program;
      //! @endignore

      //! @class CBC
      //!
      //! This class has moved to submodules of the respective ciphers.
      //!
      //! @deprecated BlockCipher.CBC

      protected CipherState proxy_obj;

      protected CipherState substate_factory()
      {
	return proxy_obj;
      }

      //!
      protected __deprecated__ void create(CipherState|program(CipherState) fun,
					   mixed ... args)
      {
	if (callablep(fun)) {
	  proxy_obj = [object(CipherState)]fun(@args);
	} else if (objectp(fun)) {
	  proxy_obj = [object(CipherState)]fun;
	} else {
	  error("Bad argument 1 to create(). Expected program|object|function.\n");
	}
	compat_assert_is_crypto_object(proxy_obj);
	::create();
      }

      //! @endclass

      //! @ignore
    }
  }

  class _Buffer {
    inherit ::this_program;

    class State
    {
      inherit ::this_program;

      //! @endignore

      //! @class Buffer
      //!
      //! This class has moved to submodules of the respective ciphers.
      //!
      //! @deprecated BlockCipher.Buffer

      protected CipherState proxy_obj;

      protected CipherState substate_factory()
      {
	return proxy_obj;
      }

      //!
      protected __deprecated__ void create(CipherState|program(CipherState) fun,
					   mixed ... args)
      {
	if (callablep(fun)) {
	  proxy_obj = [object(CipherState)]fun(@args);
	} else if (objectp(fun)) {
	  proxy_obj = [object(CipherState)]fun;
	} else {
	  error("Bad argument 1 to create(). Expected program|object|function.\n");
	}
	compat_assert_is_crypto_object(proxy_obj);
	::create();
      }

      //! @endclass

      //! @ignore
    }
  }
}

//! @endignore

protected CompatProxy compat_proxy = CompatProxy();

__deprecated__ program(CipherState) CBC =
  [__deprecated__(program)]compat_proxy.CBC.State;

//! This class has moved to submodules of the respective ciphers.
__deprecated__ program(CipherState) Buffer =
  [__deprecated__(program)]compat_proxy.Buffer.State;

#pragma deprecation_warnings
