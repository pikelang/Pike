#define TRACE werror("### %s(%d)\n", __FILE__, __LINE__);
#define SHOW(x) werror("### %s(%d) %s == %O\n", __FILE__, __LINE__, #x, (x));
