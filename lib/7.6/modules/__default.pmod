// Compatibility namespace
// $Id: __default.pmod,v 1.2 2006/03/22 17:55:22 grubba Exp $

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

mapping(string:mixed) all_constants()
{
  mapping(string:mixed) ret = predef::all_constants()+([]);

  ret->_describe_program = _describe_program;
  ret->sprintf = sprintf_76;
  return ret;
}
