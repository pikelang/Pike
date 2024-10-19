#pike __REAL_VERSION__

//! This wrapper can be placed around another object to get
//! printouts about what is happening to it. Only a few LFUNs
//! are currently supported.
//! @example
//!  > object x=Debug.Wrapper(Crypto.MD5());
//!  Debug.Wrapper is proxying ___Nettle.MD5_State()
//!  > x->name();
//!  ___Nettle.MD5_State()->name
//!  (1) Result: "md5"
//!  > !x;
//!  !___Nettle.MD5_State()
//!  (2) Result: 0

protected object wrappee;
protected object compile_handler;

//!
protected void create(object x) {
  werror("Debug.Wrapper is proxying %O\n", x);
  wrappee = x;

  compile_handler = class {
    mapping(string:mixed) get_default_module() {
      return all_constants() + ([ "wrappee":wrappee ]);
    }
  }();
}

//!
protected int(0..1) `!()
{
  werror("!%O\n", wrappee);
  return !wrappee;
}

//!
protected mixed `[](mixed x, void|mixed y) {
  if(undefinedp(y)) {
    werror("%O[%O]\n", wrappee, x);
    return wrappee[x];
  }
  else {
    werror("%O[%O..%O]\n", wrappee, x, y);
    return wrappee[x..y];
  }
}

//!
protected mixed `->(mixed x) {
  if(stringp(x))
    werror("%O->%s\n", wrappee, x);
  else
    error("%O->`->(%O) doesn't work now.\n", wrappee, x);
  return compile_string("mixed foo() { return wrappee->"+x+"; }",
			0, compile_handler)()->foo();
}

//!
protected array _indices() {
  werror("indices(%O)\n", wrappee);
  return indices(wrappee);
}

//!
protected array _values() {
  werror("values(%O)\n", wrappee);
  return values(wrappee);
}

//!
protected int _sizeof() {
  werror("sizeof(%O)\n", wrappee);
  return sizeof(wrappee);
}

//!
protected string _sprintf(int c, mapping(string:mixed)|void attrs)
{
  if (wrappee->_sprintf)
    return sprintf("Debug.Wrapper(%s)", wrappee->_sprintf(c, attrs));
  return sprintf(sprintf("Debug.Wrapper(%%%c)", c), wrappee);
}
