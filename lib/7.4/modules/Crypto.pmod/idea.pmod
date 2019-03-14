#pike 7.5

#if constant(Nettle.IDEA)
inherit Nettle.IDEA;

//! This is the Pike 7.4 compatibility implementation of
//! the IDEA cipher.
//!
//! @deprecated Crypto.IDEA
class _module_value {

  //! @ignore
  inherit State;
  //! @endignore

  //! @decl inherit Nettle.IDEA.State

  //!
  string name() { return "IDEA"; }

  int query_key_length() { return 16; }
  int query_block_size() { return block_size(); }
  string crypt_block(string p) { return crypt(p); }
}
#endif
