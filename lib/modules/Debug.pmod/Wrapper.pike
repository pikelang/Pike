
static object wrappee;
static object compile_handler;

void create(object x) {
  werror("Debug.Wrapper is proxying %O\n", x);
  wrappee = x;

  compile_handler = class {
    mapping(string:mixed) get_default_module() {
      return all_constants() + ([ "wrappee":wrappee ]);
    }
  }();
}

mixed `[](mixed x, void|mixed y) {
  if(zero_type(y)) {
    werror("%O[%O]\n", wrappee, x);
    return wrappee[x];
  }
  else {
    werror("%O[%O..%O]\n", wrappee, x, y);
    return wrappee[x..y];
  }
}

mixed `->(mixed x) {
  if(stringp(x))
    werror("%O->%s\n", wrappee, x);
  else
    error("%O->`->(%O) doesn't work now.\n", wrappee, x);
  return compile_string("mixed foo() { return wrappee->"+x+"; }",
			0, compile_handler)()->foo();
}

array _indices() {
  werror("indices(%O)\n", wrappee);
  return indices(wrappee);
}

array _values() {
  werror("values(%O)\n", wrappee);
  return values(wrappee);
}

int _sizeof() {
  werror("sizeof(%O)\n", wrappee);
  return sizeof(wrappee);
}
