#pike 7.5

#if constant(Nettle.SHA1_State)
inherit Nettle.SHA1_State;

string hash(string m) { return update(m)->digest(); }
string name() { return "SHA"; }
#elif constant(Crypto.sha);
inherit Crypto.sha;
#endif
