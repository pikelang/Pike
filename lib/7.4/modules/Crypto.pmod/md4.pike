#pike 7.5

#if constant(Nettle.MD4_State)
inherit Nettle.MD4_State;

string identifier() { return "*\206H\206\367\r\2\4"; }
string asn1_id() { return identifier(); }
string hash(string m) { return update(m)->digest(); }
string name() { return "MD4"; }
#endif
