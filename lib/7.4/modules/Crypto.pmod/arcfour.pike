#pike 7.5

//! @deprecated Crypto.Arcfour

#if constant(Nettle.ARCFOUR_State)
inherit Nettle.ARCFOUR_State;

string name() { return "ARCFOUR"; }

int query_key_length() { return 1; }
string crypt_block(string p) { return crypt(p); }
#endif
