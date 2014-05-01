
#pike 8.0

//! Use @expr{Crypto.DES.CBC@} instead.
//! @deprecated

#if constant(Crypto.DES.CBC)

inherit Crypto.DES.CBC.State;
string crypt_block(string data) { return crypt(data); }
int query_key_length() { return key_size(); }
int query_block_size() { return block_size(); }

#endif
