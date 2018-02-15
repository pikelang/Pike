#pike 7.5

//! @deprecated Crypto.DES

#if constant(Nettle.DES_State)
inherit Nettle.DES_State;

string name() { return "DES"; }

int query_key_length() { return 8; }
int query_block_size() { return 8; }
string crypt_block(string p) { return crypt(p); }
#endif
