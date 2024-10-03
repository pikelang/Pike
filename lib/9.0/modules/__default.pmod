// Compatibility namespace

#pike 9.1

//! Pike 9.0 compatibility.
//!
//! The symbols in this namespace will appear in
//! programs that use @tt{#pike 9.0@} or lower.

//! @decl inherit 9.1::

object master()
{
  return __REAL_VERSION__::master()->get_compat_master(9, 0);
}

protected Mapping.ShadowedMapping compat_all_constants =
  Mapping.ShadowedMapping(predef::all_constants(),
                          ([
                            "add_constant": add_constant,
                            "all_constants": all_constants,
                            "master": master,
                          ]), 2);

mapping(string:mixed) all_constants()
{
  // Intentional lie in the return type.
  mixed x = compat_all_constants;
  return x;
}

void add_constant(string sym, mixed|void val)
{
  if (undefinedp(val)) m_delete(compat_all_constants, sym);
  else compat_all_constants[sym] = val;
}
