#pike __REAL_VERSION__
#pragma strict_types

/* Old crypto module */
//! @ignore
inherit _Crypto;
//! @endignore

#if constant(Nettle.HashInfo)

class HashState {
  inherit Nettle.HashState;
  this_program update(string); // Better return type
}

//! Abstract class for hash algorithms. Contains some tools useful
//! for all hashes.
class Hash
{
  inherit Nettle.HashInfo;

  //! Calling `() will return a @[HashState] object.
  HashState `()();
  
  //! @decl string hash(string data)
  //!
  //!  Works as a shortcut for @expr{obj->update(data)->digest()@}.
  //!
  //! @seealso
  //!   @[HashState()->update()] and @[HashState()->digest()].
  string hash(string data)
  {
    return `()()->update(data)->digest();
  }
}

//! @decl string crypt_md5(string password, void|int salt)
//! @decl int(0..1) crypt_md5(string password, string hash)
//! Hashes a @[password] together with a @[salt] with the
//! crypt_md5 algorithm and returns the result. If such an
//! result is provided as @[hash], the password will instead
//! be hashed and compared with the @[hash], returning either
//! @expr{1@} or @expr{0@}.
string|int(0..1) crypt_md5(string password, void|string salt) {
  if(salt) {
    if(has_prefix(salt, "$1$")) {
      // Verify:
      string hash, full=salt;
      if( sscanf(full, "$1$%s$%s", salt, hash)!=2 )
	error("Error in hash.\n");
      return Nettle.crypt_md5(password, salt)==hash;
    }
    sscanf(salt, "%s$", salt);
  }
  else
    salt = ([function(string:string)]MIME["encode_base64"])
      (.Random.random_string(8));

  return "$1$"+salt+"$"+Nettle.crypt_md5(password, salt);
}

class CipherState {
  inherit Nettle.CipherState;
  this_program set_encrypt_key(string); // Better return type
  this_program set_decrypt_key(string); // Better return type
}

//! Abstract class for crypto algorithms. Contains some tools useful
//! for all ciphers.
class Cipher
{
  inherit Nettle.CipherInfo;

  //! Calling `() will return a @[CipherState] object.
  CipherState `()();

  //! Works as a shortcut for @expr{obj->set_encrypt_key(key)->crypt(data)@}
  string encrypt(string key, string data) {
    return `()()->set_encrypt_key(key)->crypt(data);
  }

  //! Works as a shortcut for @expr{obj->set_decrypt_key(key)->crypt(data)@}
  string decrypt(string key, string data) {
    return `()()->set_decrypt_key(key)->crypt(data);
  }
}

constant CBC = Nettle.CBC;
constant Proxy = Nettle.Proxy;

#endif /* constant(Nettle.HashInfo) */
