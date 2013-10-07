#pike 7.5

//! This is the Pike 7.4 compatibility implementation of the MD4 hash algorithm.
//!
//! @deprecated Crypto.MD4

#if constant(Nettle.MD4)
//! @ignore
inherit Nettle.MD4;

constant _module_value = class {
    inherit MD4::State;
    //! @endignore

    //! @decl inherit Nettle.MD4.State

    string identifier() { return "*\206H\206\367\r\2\4"; }
    string asn1_id() { return identifier(); }
    //! Return the BER encoded ASN.1 identifier for MD4.

    //! @deprecated Crypto.MD4.hash
    string hash(string m) { return update(m)->digest(); }

    //!
    string name() { return "MD4"; }

    //! @ignore
  };
//! @endignore
#endif
