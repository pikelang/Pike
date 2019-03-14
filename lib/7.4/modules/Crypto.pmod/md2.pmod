#pike 7.5

#if constant(Nettle.MD2)
inherit Nettle.MD2;

//! This is the Pike 7.4 compatibility implementation of the MD2 hash algorithm.
//!
//! @deprecated Crypto.MD2
class _module_value {

  //! @ignore
  inherit MD2::State;
  //! @endignore

  //! @decl inherit Nettle.MD2.State

  string identifier() { return "*\206H\206\367\r\2\2"; }
  string asn1_id() { return identifier(); }
  //! Return the BER encoded ASN.1 identifier for MD2.

  //! @deprecated Crypto.MD2.hash
  string hash(string m) { return update(m)->digest(); }

  //!
  string name() { return "MD2"; }
}
#endif
