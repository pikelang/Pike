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

#pragma no_deprecation_warnings

//! Wrapper for iterators that implements the 8.0 and
//! earlier iterator API.
protected class Iterator
{
  protected predef::Iterator it;

  protected mixed index_value;

  protected void create(predef::Iterator it)
  {
    this::it = it;
    index_value = iterator_next(it);
  }

  protected int(0..1) `!()
  {
    return undefinedp(index_value);
  }

  int(0..1) next()
  {
    index_value = iterator_next(it);
    return !undefinedp(index_value);
  }

  mixed index()
  {
    catch {
      return iterator_index(it);
    };
    return index_value;
  }

  mixed value()
  {
    catch {
      return iterator_value(it);
    };
    return index_value;
  }

  //
  // Optional identifiers from this point onwards.
  //

  protected int _sizeof()
  {
    return sizeof(it);
  }

  protected void _random()
  {
    random(it);

    index_value = iterator_next(it);
  }

  protected int(0..1) internal_first()
  {
    if (it->first) {
      it->first();
    }

    index_value = iterator_next(it);

    return !undefinedp(index_value);
  }

  protected function(:mixed)|zero `first()
  {
    if (it->first) return internal_first;
    return UNDEFINED;
  }

  protected void internal_set_index(mixed index)
  {
    if (it->set_index) {
      it->set_index(index);
    }

    index_value = iterator_next(it);
  }

  protected function(mixed:void)|zero `set_index()
  {
    if (it->set_index) return internal_set_index;
    return UNDEFINED;
  }
}

#pragma deprecation_warnings

protected Iterator get_iterator(mixed x)
{
  return Iterator(predef::get_iterator(x));
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
			    "get_iterator": get_iterator,
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
			    "array_sscanf": array_sscanf_80,
			    "werror" : werror,
			    "exit" : exit,
			  ]), 1);

mapping(string:mixed) all_constants()
{
  // Intentional lie in the return type.
  mixed x = compat_all_constants;
  return x;
}
