#pike 7.5
#pragma strict_types
// $Id$

//! Inversion crypto module.

//! Returns the string @expr{"INVERT"@}.
string name() { return "INVERT"; }

//! Returns the block size for the invert crypto (currently 8).
int(8..8) query_block_size() { return 8; }

//! Returns the minimum key length for the invert crypto.
//!
//! Since this crypto doesn't use a key, this will be 0.
int(0..0) query_key_length() { return 0; }

//! Set the encryption key (currently a no op).
void set_encrypt_key(string data) {}

//! Set the decryption key (currently a no op).
void set_decrypt_key(string data) {}

//! De/encrypt the string @[data] with the invert crypto
//! (ie invert the string).
string crypt_block(string data) {
  if(sizeof(data)%8)
    error("Bad length of data.\n");
  return ~data;
}
