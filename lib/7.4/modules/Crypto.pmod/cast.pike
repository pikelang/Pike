#pike 7.5

#if constant(Nettle.CAST128_State)
inherit Nettle.CAST128_State;

string name() { return "CAST"; }

int query_key_length() { return 16; }
int query_block_size() { return block_size(); }
string crypt_block(string p) { return crypt(p); }
#elif constant(Crypto.cast)
inherit Crypto.cast;
#endif
