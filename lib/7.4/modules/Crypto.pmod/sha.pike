#pike 7.5

//! @deprecated Crypto.SHA1

#if constant(Nettle.SHA1_State)
inherit Nettle.SHA1_State;

string hash(string m) { return update(m)->digest(); }
string name() { return "SHA"; }
#endif
