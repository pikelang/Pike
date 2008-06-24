// Compatibility namespace
// $Id: __default.pmod,v 1.9 2008/06/24 12:44:46 mast Exp $

#pike 7.7

//! Pike 7.6 compatibility.
//!
//! The symbols in this namespace will appear in
//! programs that use @tt{#pike 7.6@} or lower.

//! @decl inherit predef::

static array(array(int|string)) _describe_program(program p)
{
  return map(predef::_describe_program(p),
	     lambda(array(mixed) symbol_info) {
	       // Remove the value_type entry (index 2).
	       return symbol_info[..1]+symbol_info[3..];
	     });
}

object master()
{
  return __REAL_VERSION__::master()->get_compat_master(7, 6);
}

static object compat_all_constants =
  __REAL_VERSION__::master()->CompatAllConstants (
    (["all_constants": all_constants,
      "_describe_program": _describe_program,
      "sprintf": sprintf_76,
      "array_sscanf": array_sscanf_76,
      "master": master,
    ]));

mapping(string:mixed) all_constants()
{
  // Intentional lie in the return type.
  mixed x = compat_all_constants;
  return x;
}
