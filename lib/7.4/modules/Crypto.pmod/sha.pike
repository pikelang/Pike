#pike 7.5

#if constant(Nettle.SHA1_State)
inherit Nettle.SHA1_State;

string name() { return "SHA"; }
#elif constant(Crypto.sha);
inherit Crypto.sha;
#endif
