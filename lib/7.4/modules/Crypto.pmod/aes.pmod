#pike 7.5

#if constant(Nettle.AES)
inherit Nettle.AES;

//! This is the Pike 7.4 compatibility implementation of
//! the AES cipher.
//!
//! @deprecated Crypto.AES
class _module_value {

  //! @ignore
  inherit State;
  //! @endignore

  //! @decl inherit Nettle.AES.State

  //!
  string name() { return "AES"; }

  int query_key_length() { return 32; }
  int query_block_size() { return block_size(); }
  string crypt_block(string p) { return crypt(p); }
}
#endif
