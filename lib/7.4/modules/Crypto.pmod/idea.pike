#pike 7.5

//! @deprecated Crypto.IDEA

#if constant(Nettle.IDEA_State)
inherit Nettle.IDEA_State;

string name() { return "IDEA"; }

int query_key_length() { return 16; }
int query_block_size() { return block_size(); }
string crypt_block(string p) { return crypt(p); }
#endif
