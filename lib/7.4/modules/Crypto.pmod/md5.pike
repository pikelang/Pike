#pike 7.5

#if constant(Nettle.MD5_State)
inherit Nettle.MD5_State;

string identifier() { return "*\206H\206\367\r\2\5"; }
string name() { return "MD5"; }
#elif constant(Crypto.md5)
inherit Crypto.md5;
#endif
