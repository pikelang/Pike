#pike __REAL_VERSION__

/* Old crypto module */
inherit _Crypto;

#if constant(Nettle.HashInfo)

import Nettle;

//!
class Hash
{
  inherit HashInfo;

  HashState `()();
  
  //! @decl string hash(string data)
  //!
  //!  Works as a shortcut for @expr{obj->update(data)->digest()@}.
  //!
  //! @note
  //!  The hash buffer will not be cleared before @[data] is added
  //!  to the buffer, so data added with calls to @[update] will be
  //!  prepended to the @[data].
  //!
  //! @seealso
  //!   @[update] and @[digest].
  string hash(string data)
  {
    return `()()->update(data)->digest();
  }
}

class MD5_Algorithm
{
  // NOTE: Depends on the order of INIT invocations.
  inherit MD5_Info;
  inherit Hash;

  MD5_State `()() { return MD5_State(); }
}

MD5_Algorithm MD5 = MD5_Algorithm();

string crypt_md5(string password, void|string salt) {
  if(salt) {
    if(has_prefix(salt, "$1$")) {
      // Verify:
      string hash, full=salt;
      if( sscanf(full, "$1$%s$%s", salt, hash)!=2 )
	error("Error in hash.\n");
      if( Nettle.crypt_md5(password, salt)==hash )
	return full;
    }
    sscanf(salt, "%s$", salt);
  }
  else
    salt = MIME.encode_base64(random_string(8));
  return "$1$"+salt+"$"+Nettle.crypt_md5(password, salt);
}

#if constant(Nettle.MD4_Info)

class MD4_Algorithm
{
  // NOTE: Depends on the order of INIT invocations.
  inherit MD4_Info;
  inherit Hash;

  MD4_State `()() { return MD4_State(); }
}

MD4_Algorithm MD4 = MD4_Algorithm();

#endif

#if constant(Nettle.MD2_Info)

class MD2_Algorithm
{
  // NOTE: Depends on the order of INIT invocations.
  inherit MD2_Info;
  inherit Hash;

  MD2_State `()() { return MD2_State(); }
}

MD2_Algorithm MD2 = MD2_Algorithm();

#endif

class SHA_Algorithm
{
  // NOTE: Depends on the order of INIT invocations.
  inherit SHA1_Info;
  inherit Hash;

  SHA1_State `()() { return SHA1_State(); }
}

SHA_Algorithm SHA = SHA_Algorithm();

class SHA256_Algorithm
{
  // NOTE: Depends on the order of INIT invocations.
  inherit SHA256_Info;
  inherit Hash;

  SHA256_State `()() { return SHA256_State(); }
}

SHA256_Algorithm SHA256 = SHA256_Algorithm();

class Cipher
{
  inherit CipherInfo;

  CipherState `()();

  string encrypt(string key, string data) {
    return `()()->set_encrypt_key(key)->crypt(data);
  }

  string decrypt(string key, string data) {
    return `()()->set_decrypt_key(key)->crypt(data);
  }
}

class AES_Algorithm
{
  // NOTE: Depends on the order of INIT invocations.
  inherit AES_Info;
  inherit Cipher;

  AES_State `()() { return AES_State(); }
}

AES_Algorithm AES = AES_Algorithm();

class ARCFOUR_Algorithm
{
  // NOTE: Depends on the order of INIT invocations.
  inherit ARCFOUR_Info;
  inherit Cipher;

  ARCFOUR_State `()() { return ARCFOUR_State(); }
}

ARCFOUR_Algorithm ARCFOUR = ARCFOUR_Algorithm();

class BLOWFISH_Algorithm
{
  // NOTE: Depends on the order of INIT invocations.
  inherit BLOWFISH_Info;
  inherit Cipher;

  BLOWFISH_State `()() { return BLOWFISH_State(); }
}

BLOWFISH_Algorithm Blowfish = BLOWFISH_Algorithm();

class CAST_Algorithm
{
  // NOTE: Depends on the order of INIT invocations.
  inherit CAST128_Info;
  inherit Cipher;

  CAST128_State `()() { return CAST128_State(); }
}

CAST_Algorithm CAST = CAST_Algorithm();

class DES_Algorithm
{
  // NOTE: Depends on the order of INIT invocations.
  inherit DES_Info;
  inherit Cipher;

  DES_State `()() { return DES_State(); }
}

DES_Algorithm DES = DES_Algorithm();

//!
class DES3_Algorithm
{
  // NOTE: Depends on the order of INIT invocations.
  inherit DES3_Info;
  inherit Cipher;

  //! Creates a valid DES3-key out of its in parameter.
  //! Two 7 or 8 bytes long strings or one 14 or 16 bytes will
  //! result in an ABA key. Thre 7 or 8 bytes long strings or one
  //! 21 or 24 bytes will result in an ABC key.
  string fix_key(string key, string|void key2, string|void key3) {
    if(key && key2 && key3)
      return DES->fix_parity(key)+DES->fix_parity(key2)+DES->fix_parity(key3);
    if(key && key2)
      return DES->fix_parity(key)+DES->fix_parity(key2)+DES->fix_parity(key);
    if(key) {
      switch(sizeof(key)) {
      case 14:
	return DES->fix_parity(key[..6])+
	  DES->fix_parity(key[7..])+
	  DES->fix_parity(key[..6]);
      case 16:
	return DES->fix_parity(key[..7])+
	  DES->fix_parity(key[8..])+
	  DES->fix_parity(key[..7]);
      case 21:
	return DES->fix_parity(key[..6])+
	  DES->fix_parity(key[7..13])+
	  DES->fix_parity(key[14..]);
      case 24:
	return DES->fix_parity(key[..7])+
	  DES->fix_parity(key[8..15])+
	  DES->fix_parity(key[16..]);
      }
    }
    error("Invalid in parameters.\n");
  }

  DES3_State `()() { return DES3_State(); }
}

DES3_Algorithm DES3 = DES3_Algorithm();

#if 0
class IDEA_Algorithm
{
  // NOTE: Depends on the order of INIT invocations.
  inherit IDEA_Info;
  inherit Cipher;

  IDEA_State `()() { return IDEA_State(); }
}

IDEA_Algorithm IDEA = IDEA_Algorithm();
#endif

class Serpent_Algorithm
{
  // NOTE: Depends on the order of INIT invocations.
  inherit Serpent_Info;
  inherit Cipher;

  Serpent_State `()() { return Serpent_State(); }
}

Serpent_Algorithm Serpent = Serpent_Algorithm();

class Twofish_Algorithm
{
  // NOTE: Depends on the order of INIT invocations.
  inherit Twofish_Info;
  inherit Cipher;

  Twofish_State `()() { return Twofish_State(); }
}

Twofish_Algorithm Twofish = Twofish_Algorithm();

#endif /* constant(Nettle.HashInfo) */
