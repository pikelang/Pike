#pike 7.5

#if constant(Nettle.DES)
inherit Nettle.DES;

//! This is the Pike 7.4 compatibility implementation of
//! the DES cipher algorithm.
//!
//! @deprecated Crypto.DES
class _module_value {

  //! @ignore
  inherit State;
  //! @endignore

  //! @decl inherit Nettle.DES.State

  //!
  string name() { return "DES"; }

  int query_key_length() { return 8; }
  int query_block_size() { return 8; }
  string crypt_block(string p) { return crypt(p); }
}
#endif
