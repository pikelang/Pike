
#pike __REAL_VERSION__

#if constant(SQLite.SQLite)
inherit SQLite.SQLite;

void create(string a, void|string b, void|mixed c, void|mixed d) {
  if(b) a += "/"+b;
  ::create(a);
}

#else
constant this_program_does_not_exist=1;
#endif
