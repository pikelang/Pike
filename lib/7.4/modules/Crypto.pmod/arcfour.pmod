#pike 7.5

#if constant(Nettle.ARCFOUR)
inherit Nettle.ARCFOUR;

//! This is the Pike 7.4 compatibility implementation of
//! the Arcfour cipher.
//!
//! @deprecated Crypto.Arcfour
class _module_value {

  //! @ignore
  inherit State;
  //! @endignore

  //! @decl inherit Nettle.ARCFOUR.State

  //!
  string name() { return "ARCFOUR"; }

  int query_key_length() { return 1; }
  string crypt_block(string p) { return crypt(p); }
}
#endif
