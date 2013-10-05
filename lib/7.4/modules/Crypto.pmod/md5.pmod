#pike 7.5

//! @deprecated Crypto.MD5

#if constant(Nettle.MD5)
inherit Nettle.MD5;

constant _module_value = class {
    inherit MD5::State;

    string identifier() { return "*\206H\206\367\r\2\5"; }
    string asn1_id() { return identifier(); }
    string hash(string m) { return update(m)->digest(); }
    string name() { return "MD5"; }
  };
#endif
