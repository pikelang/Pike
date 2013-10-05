#pike 7.5

//! @deprecated Crypto.SHA1

#if constant(Nettle.SHA1)
inherit Nettle.SHA1;

constant _module_value = class {
    inherit SHA1::State;

    string hash(string m) { return update(m)->digest(); }
    string name() { return "SHA"; }
  };
#endif
