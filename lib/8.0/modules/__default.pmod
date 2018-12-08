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

//! @decl void _assembler_debug()
//!
//! @deprecated Debug.assembler_debug

//! @decl int _debug(int d)
//!
//! @deprecated Debug.debug

//! @decl void _dump_program_tables(program p, int|void i)
//!
//! @deprecated Debug.dump_program_tables

//! @decl int _optimizer_debug(int d)
//!
//! @deprecated Debug.optimizer_debug

//! @decl int _map_all_objects(function f)
//!
//! @deprecated Debug.map_all_objects

//! @decl int _refs(mixed x)
//!
//! @deprecated Debug.refs

//! @decl mixed _next(mixed x)
//!
//! @deprecated Debug.next

//! @decl mixed _prev(mixed x)
//!
//! @deprecated Debug.prev

//! @decl object next_object(object o)
//!
//! @deprecated Debug.next_object

protected int|array(string) _glob(string|array(string) a, string|array(string) b)
{
  mapping r = ([
    "[" : "\\[",
    "\\" : "\\\\"
  ]);
  if( stringp(a) )
    a = replace(a, r);
  else
    a = replace(a[*], r);
  return 8.1::glob(a,b);
}

protected int(0..1) glob(string a, string b)
{
  return _glob(a,b);
}
variant protected int(0..1) glob(array(string) a, string b)
{
  return !!_glob(a,b);
}
variant protected array(string) glob(array(string)|string a, array(string) b)
{
  return _glob(a,b);
}

protected function werror = _static_modules._Stdio._stderr->write;

protected void exit(int rc, void | string fmt, void | mixed ... args)
{
  if (fmt)
    /* Compat werror */
    werror(fmt, @args);
  8.1::exit(rc);
}

protected Mapping.ShadowedMapping compat_all_constants =
  Mapping.ShadowedMapping(predef::all_constants(),
			  ([
#if constant(Debug.HAVE_DEBUG)
			    "_assembler_debug": Debug.assembler_debug,
			    "_debug": Debug.debug,
			    "_describe": Debug.describe,
			    "_dump_backlog": Debug.dump_backlog,
			    "_dump_program_tables": Debug.dump_program_tables,
			    "_gc_set_watch": Debug.gc_set_watch,
			    "_leak": Debug.leak,
			    "_locate_references": Debug.locate_references,
			    "_optimizer_debug": Debug.optimizer_debug,
#endif /* Debug.HAVE_DEBUG */
			    "_map_all_objects": Debug.map_all_objects,
			    "_refs": Debug.refs,
                            "_next": Debug.next,
                            "_prev": Debug.prev,
			    "hash": hash_8_0,
                            "next_object": Debug.next_object,
			    "all_constants": all_constants,
                            "master": master,
                            "glob" : glob,
#ifdef constant(System.RegGetValue)
                            "RegGetValue" : System.RegGetValue,
                            "RegGetKeyNames" : System.RegGetKeyNames,
                            "RegGetValues" : System.RegGetValues,
#endif
#ifdef constant(System.openlog)
                            "openlog" : System.openlog,
                            "syslog" : System.syslog,
                            "closelog" : System.closelog,
#endif
			    "werror" : werror,
			    "exit" : exit,
			  ]), 1);

mapping(string:mixed) all_constants()
{
  // Intentional lie in the return type.
  mixed x = compat_all_constants;
  return x;
}
