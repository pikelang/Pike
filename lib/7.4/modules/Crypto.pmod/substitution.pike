
#pike 7.5

//! @deprecated Crypto.Substitution

inherit Crypto.Substitution;

string encode(string m) { return encrypt(m); }
string decode(string m) { return decrypt(m); }
