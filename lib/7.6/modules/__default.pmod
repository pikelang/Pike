// Compatibility namespace
// $Id: __default.pmod,v 1.11 2008/06/24 20:14:10 grubba Exp $

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

static Mapping.ShadowedMapping compat_all_constants =
  Mapping.ShadowedMapping(predef::all_constants());

void add_constant(string name, mixed|void value)
{
  if (zero_type(value)) {
    m_delete(compat_all_constants, name);
  } else {
    compat_all_constants[name] = value;
  }
}

mapping(string:mixed) all_constants()
{
  // Intentional lie in the return type.
  mixed x = compat_all_constants;
  return x;
}

static void create()
{
  // add_constant("add_constant", add_constant);
  add_constant("all_constants", all_constants);
  add_constant("_describe_program", _describe_program);
  add_constant("sprintf", sprintf_76);
  add_constant("array_sscanf", array_sscanf_76);
  add_constant("master", master);
}
