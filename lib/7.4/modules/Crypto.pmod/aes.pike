#pike 7.5

#if constant(Nettle.AES_State)
inherit Nettle.AES_State;

string name() { return "AES"; }

int query_key_length() { return 32; }
int query_block_size() { return block_size(); }
string crypt_block(string p) { return crypt(p); }
#elif constant(Crypto.rijndael)
inherit Crypto.rijndael;

string name() { return "AES"; }
#endif
