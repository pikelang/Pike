// Compatibility namespace
// $Id: __default.pmod,v 1.4 2008/05/17 14:10:02 marcus Exp $

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

// This replacement for the master is necessary to make code like
// master()->resolv(...) look things up in the 7.6 compat tree.
static class Master76
{
  inherit "/master";

  static object master_76_compat_handler =
    get_compilation_handler (7, 6);

  mixed resolv (string identifier, string|void current_file,
		object|void current_handler)
  {
    return master_76_compat_handler->resolv (identifier, current_file,
					     current_handler);
  }

  static void create()
  {
    object m = master();
    ::create();
    compat_major = m->compat_major;
    compat_minor = m->compat_minor;
    fc = m->fc;
    currentversion = m->currentversion;
    compat_handler_cache = m->compat_handler_cache;
  }

  static string _sprintf(int t) {return t == 'O' && "master_76()";}
}

static object master_76;

static object get_master_76()
{
  if (master_76) return master_76;
  return master_76 = Master76();
}

mapping(string:mixed) all_constants()
{
  mapping(string:mixed) ret = predef::all_constants()+([]);

  ret->_describe_program = _describe_program;
  ret->sprintf = sprintf_76;
  ret->array_sscanf = array_sscanf_76;
  ret->master = get_master_76;
  return ret;
}
