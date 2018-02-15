#pike 7.5

//! @deprecated Crypto.AES

#if constant(Nettle.AES_State)
inherit Nettle.AES_State;

string name() { return "RIJNDAEL"; }

int query_key_length() { return 32; }
int query_block_size() { return block_size(); }
string crypt_block(string p) { return crypt(p); }
#endif
