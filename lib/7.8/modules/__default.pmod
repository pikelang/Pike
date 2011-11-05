// Compatibility namespace

#pike 7.9

//! Pike 7.8 compatibility.
//!
//! The symbols in this namespace will appear in
//! programs that use @tt{#pike 7.8@} or lower.

//! @decl inherit predef::

protected Mapping.ShadowedMapping compat_all_constants =
  Mapping.ShadowedMapping(predef::all_constants(),
			  ([
			  ]), 1);

mapping(string:mixed) all_constants()
{
  // Intentional lie in the return type.
  mixed x = compat_all_constants;
  return x;
}
