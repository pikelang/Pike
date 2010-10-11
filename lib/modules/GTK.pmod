#pike __REAL_VERSION__
#if constant(GTK2.Widget)
inherit GTK2;
#elif constant(GTK1.Widget)
inherit GTK1;
#else
constant module_value = UNDEFINED;
#endif

