#pike 7.5

#if constant(Nettle.MD2_State)
inherit Nettle.MD2_State;

string identifier() { return "*\206H\206\367\r\2\2"; }
string name() { return "MD2"; }
#elif constant(Crypto.md2)
inherit Crypto.md2;
#endif
