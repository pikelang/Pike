#pike 7.5

inherit Crypto.Pipe;

string name() { return upper_case(::name()); }

array(int) query_key_length() { return key_size(); }
int query_block_size() { return block_size(); }
string crypt_block(string p) { return crypt(p); }
