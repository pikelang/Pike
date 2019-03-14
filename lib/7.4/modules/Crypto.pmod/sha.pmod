#pike 7.5

#if constant(Nettle.SHA1)
inherit Nettle.SHA1;

//! This is the Pike 7.4 compatibility implementation of
//! the SHA1 hash algorithm.
//!
//! @deprecated Crypto.SHA1
class _module_value {

  //! @ignore
  inherit SHA1::State;
  //! @endignore

  //! @decl inherit Nettle.SHA1.State

  //! @deprecated Crypto.SHA1.hash
  string hash(string m) { return update(m)->digest(); }

  //!
  string name() { return "SHA"; }
}
#endif
