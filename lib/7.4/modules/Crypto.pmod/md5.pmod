#pike 7.5

//! This is the Pike 7.4 compatibility implementation of the MD5 hash algorithm.
//!
//! @deprecated Crypto.MD5

#if constant(Nettle.MD5)
//! @ignore
inherit Nettle.MD5;

constant _module_value = class {
    inherit MD5::State;
    //! @endignore

    //! @decl inherit Nettle.MD5.State

    string identifier() { return "*\206H\206\367\r\2\5"; }
    string asn1_id() { return identifier(); }
    //! Return the BER encoded ASN.1 identifier for MD5.

    //! @deprecated Crypto.MD5.hash
    string hash(string m) { return update(m)->digest(); }

    //!
    string name() { return "MD5"; }

    //! @ignore
  };
//! @endignore
#endif
