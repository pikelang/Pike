#pike 7.5

inherit Nettle.AES_State;

string name() { return "AES"; }

int query_key_length() { return key_size(); }
int query_block_size() { return block_size(); }
string crypt_block(string p) { return crypt(p); }
