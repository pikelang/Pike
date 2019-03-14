
#pike 8.0

//! Use @expr{Crypto.DES3.CBC@} instead.
//! @deprecated

#if constant(Crypto.DES3.CBC)

inherit Crypto.DES3.CBC.State;
string crypt_block(string data) { return crypt(data); }
int query_key_length() { return key_size(); }
int query_block_size() { return block_size(); }

#endif
