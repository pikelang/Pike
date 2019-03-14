#pike 7.5

#if constant(Nettle.DES3)
inherit Nettle.DES3;

//! This is the Pike 7.4 compatibility implementation of
//! the DES3 cipher.
//!
//! @deprecated Crypto.DES3
class _module_value {

  //! @ignore
  inherit State;
  //! @endignore

  //! @decl inherit Crypto.DES3.State

  //!
  string name() { return "DES"; } // Yep, it doesn't say DES3

  array(int) query_key_length() { return ({ 8, 8, 8 }); }
  int query_block_size() { return 8; }
  int query_key_size() { return 16; }
  string crypt_block(string p) { return crypt(p); }
}
#endif
