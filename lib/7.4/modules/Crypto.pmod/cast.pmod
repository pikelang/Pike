#pike 7.5

#if constant(Nettle.CAST128)
inherit Nettle.CAST128;

//! This is the Pike 7.4 compatibility implementation of
//! the CAST128 cipher.
//!
//! @deprecated Crypto.CAST
class _module_value {

  //! @ignore
  inherit State;
  //! @endignore

  //! @decl inherit Nettle.CAST128.State;

  //!
  string name() { return "CAST"; }

  int query_key_length() { return 16; }
  int query_block_size() { return block_size(); }
  string crypt_block(string p) { return crypt(p); }
}
#endif
