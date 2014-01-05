#pike __REAL_VERSION__
#pragma strict_types

//! Base class for cipher algorithms.
//!
//! Implements some common convenience functions, and prototypes.
//!
//! Note that no actual cipher algorithm is implemented
//! in the base class. They are implemented in classes
//! that inherit this class.

//! Returns a human readable name for the algorithm.
string(8bit) name();

//! @returns
//!   The recommended key size for the cipher.
int(0..) key_size();

//! @returns
//!   The block size of the cipher (@expr{1@} for stream ciphers).
int(0..) block_size();

//! This is the context for a single incrementally updated cipher.
//!
//! Most of the functions here are only prototypes, and need to
//! be overrided via inherit.
class State
{
  //! Initializes the object for encryption.
  //!
  //! @seealso
  //!   @[set_decrypt_key], @[crypt]
  this_program set_encrypt_key(string(8bit) key, void|int force);

  //! Initializes the object for decryption.
  //!
  //! @seealso
  //!   @[set_encrypt_key], @[crypt]
  this_program set_decrypt_key(string(8bit) key, void|int force);

  protected function(int:string(8bit)) random_string;

  //! Generate a key by calling @[Crypto.Random.random_string] and
  //! initialize the object for encryption with that key.
  //!
  //! @returns
  //!   The generated key.
  //!
  //! @seealso
  //!   @[set_encrypt_key]
  string(8bit) make_key()
  {
    if (!random_string) {
      object random = [object]master()->resolv("Crypto.Random");
      random_string = [function(int:string(8bit))]random->random_string;
    }
    string(8bit) key = random_string(global::key_size());
    set_encrypt_key(key);
    return key;
  }

  //! Encrypts or decrypts data, using the current key. Neither the
  //! input nor output data is automatically memory scrubbed,
  //! unless @[String.secure] has been called on them.
  //!
  //! @param data
  //!   For block ciphers, data must be an integral number of blocks.
  //!
  //! @returns
  //!   The encrypted or decrypted data.
  string(8bit) crypt(string(8bit) data);

  //! @returns
  //!   The actual key size for this cipher.
  int(0..) key_size();
}

//! Calling `() will return a @[State] object.
State `()() { return State(); }

//! Works as a shortcut for @expr{obj->set_encrypt_key(key)->crypt(data)@}
string encrypt(string(8bit) key, string(8bit) data) {
  return `()()->set_encrypt_key(key)->crypt(data);
}

//! Works as a shortcut for @expr{obj->set_decrypt_key(key)->crypt(data)@}
string decrypt(string(8bit) key, string(8bit) data) {
  return `()()->set_decrypt_key(key)->crypt(data);
}
