#pike 7.5

//! This is the Pike 7.4 compatibility implementation of
//! the SHA1 hash algorithm.
//!
//! @deprecated Crypto.SHA1

#if constant(Nettle.SHA1)
//! @ignore
inherit Nettle.SHA1;

constant _module_value = class {
    inherit SHA1::State;
    //! @endignore

    //! @decl inherit Nettle.SHA1.State

    //! @deprecated Crypto.SHA1.hash
    string hash(string m) { return update(m)->digest(); }

    //!
    string name() { return "SHA"; }

    //! @ignore
  };
//! @endignore
#endif
