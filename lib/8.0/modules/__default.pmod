// Compatibility namespace

#pike 8.1

//! Pike 8.0 compatibility.
//!
//! The symbols in this namespace will appear in
//! programs that use @tt{#pike 8.0@} or lower.

//! @decl inherit predef::

object master()
{
  return __REAL_VERSION__::master()->get_compat_master(8, 0);
}

protected Mapping.ShadowedMapping compat_all_constants =
  Mapping.ShadowedMapping(predef::all_constants(),
			  ([
			    "master": master,
			  ]), 1);

mapping(string:mixed) all_constants()
{
  // Intentional lie in the return type.
  mixed x = compat_all_constants;
  return x;
}
