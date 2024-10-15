// Compatibility namespace

#pike 8.1

//! Pike 8.0 compatibility.
//!
//! The symbols in this namespace will appear in
//! programs that use @tt{#pike 8.0@} or lower.

//! @decl inherit 8.1::

object master()
{
  return __REAL_VERSION__::master()->get_compat_master(8, 0);
}

#pragma no_deprecation_warnings

//! Wrapper for iterators that implements the 8.0 and
//! earlier iterator API. This class doubles as the
//! interface for old-style iterators.
//!
//! The symbols required to be implemented in this API are:
//! @ul
//!   @item @[`!()]
//!     Check whether the iterator points to a valid item.
//!
//!   @item @[index()]
//!     Get the current iterator index.
//!
//!   @item @[value()]
//!     Get the current iterator value.
//!
//!   @item @[next()]
//!   @item @[`+=()]
//!     Advance the iterator. One of them may be omitted.
//!     @[next()] is preferred by @[foreach()].
//! @endul
//!
//! @deprecated predef::Iterator
//!
//! @seealso
//!   @[predef::get_iterator()], @[lfun::_get_iterator()],
//!   @[predef::Iterator]
class Iterator
{
  protected predef::Iterator it;

  protected mixed index_value;

  //! Initialize this iterator to wrap the @[predef::Iterator] in @[it]
  //!
  //! The iterator initially points to the first item in the data set,
  //! if there is any.
  //!
  //! @note
  //!   This differs from the @[predef::Iterator] API, where the
  //!   iterator is initialized to point before the first element.
  protected void create(predef::Iterator it)
  {
    this::it = it;
    index_value = iterator_next(it);
  }

  //! Returns @expr{0@} (zero) when the iterator points to an item,
  //! @expr{1@} otherwise.
  //!
  //! @seealso
  //!   @[lfun::`!()]
  protected int(0..1) `!()
  {
    return undefinedp(index_value);
  }

  //! If this function is defined it should advance the iterator one
  //! step, just like @expr{@[`+=](1)@} would do.
  //!
  //! @note
  //!   This is the preferred way to advance the iterator, since it
  //!   reduces the overhead.
  //!
  //! @note
  //!   This function was optional in Pike 7.6 and earlier.
  //!
  //! @returns
  //!   Returns @tt{1@} if it succeeded in advancing, and
  //!   @tt{0@} (zero) if it has reached the end of the iterator.
  //!
  //! @seealso
  //!   @[`+], @[`+=], @[`-], @[lfun::_iterator_next()]
  optional int(0..1) next()
  {
    index_value = iterator_next(it);
    return !undefinedp(index_value);
  }

  //! Advance this iterator the specified number of steps and return
  //! it. The amount may be negative to move backwards. If the
  //! iterator doesn't support backward movement it should throw an
  //! exception in that case.
  //!
  //! @note
  //!   @[foreach] calls this function with a step value of @expr{1@}
  //!   if @[next()] doesn't exist for compatibility with Pike 7.6
  //!   and earlier.
  //!
  //! @note
  //!   @[foreach] will call this function even when the the
  //!   iterator has more than one reference. If you want to
  //!   loop over a copy of the iterator, you can create a
  //!   copy by adding @expr{0@} (zero) to it:
  //!   @code
  //!     Iterator iterator;
  //!     ...
  //!     foreach(iterator+0; mixed index; mixed value) {
  //!       ...
  //!     }
  //!   @endcode
  //!
  //! @note
  //!   Even though this function is sufficient for @[foreach] to
  //!   advance the iterator, @[next()] is the preferred API.
  //!   @[next()] has the additional advantage of not being an
  //!   lfun, so it is possible to advance the iterator by hand.
  //!
  //! @seealso
  //!   @[next], @[`+], @[`-]
  optional protected int(0..1) `+=(steps)
  {
    if (steps < 0) error("Stepping backwards not supported.\n");
    if (!steps) return !undefinedp(index_value);
    while (steps--) {
      index_value = iterator_next(it);
      if (undefinedp(index_value)) return 0;
    }
    return !undefinedp(index_value);
  }

  //! Returns a clone of this iterator which is advanced the specified
  //! number of steps. The amount may be negative to move backwards.
  //! If the iterator doesn't support backward movement it should
  //! throw an exception in that case.
  //!
  //! @returns
  //!   Returns a new @[Iterator] positioned at the new place.
  //!
  //! @seealso
  //!   @[next], @[`+=], @[`-]
  optional protected Iterator `+(int steps);

  //! This lfun should be defined if and only if the iterator supports
  //! backward movement to some degree. It should back up the
  //! specified number of steps. The amount may be negative to move
  //! forward.
  //!
  //! @returns
  //!   Returns a new @[Iterator] positioned at the new place.
  //!
  //! @seealso
  //!   @[next], @[`+], @[`+=]
  optional protected Iterator `-(int steps);

  //! Returns the current index, or @[UNDEFINED] if the iterator
  //! doesn't point to any item.
  //!
  //! If there's no obvious index set then the index is the current
  //! position in the data set, counting from @expr{0@} (zero).
  mixed index()
  {
    catch {
      return iterator_index(it);
    };
    return index_value;
  }

  //! Returns the current value, or @[UNDEFINED] if the iterator
  //! doesn't point to any item.
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

  //! Returns the total number of items in the data set according to
  //! this iterator. If the size of the data set is unlimited or
  //! unknown then this function shouldn't be defined.
  optional protected int _sizeof()
  {
    return sizeof(it);
  }

  //! If this function is defined then it sets the iterator to point
  //! to a random item in the accessible set. The random distribution
  //! should be rectangular within that set, and the pseudorandom
  //! sequence provided by @[random] should be used.
  optional protected void _random()
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

  //! @decl optional int(0..1) first()
  //!
  //! If this function is defined then it resets the iterator to point
  //! to the first item.
  //!
  //! @returns
  //!   Returns zero if there are no items at all in
  //!   the data set, one otherwise.
  //!
  //! @note
  //!   It's not enough to set the iterator to the earliest accessible
  //!   item. If the iterator doesn't support backing up to the
  //!   original start position then this function should not be
  //!   implemented.

  optional function(:mixed)|zero `first()
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

  //! @decl optional void set_index(zero index)
  //!
  //! If this function is defined it should set the iterator at
  //! the specified index.
  //!
  //! @note
  //!   It should be possible to set the index at the end of
  //!   the iterator.

  optional function(mixed:void)|zero `set_index()
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

//! Backward compatibility implementation of @[predef::glob()].
//!
//! This function differs from @[predef::glob()] in that the
//! characters @expr{'\\'@} and @expr{'['@} are NOT special,
//! which means that it is not possible to quote any of the
//! special characters nor are character ranges supported.
//!
//! Do NOT use unless you know what you are doing.
//!
//! @seealso
//!   @[predef::glob()]
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

//! Backward compatibility implementation of @[predef::compile()].
//!
//! This only differs from @[predef::compile()] in that the
//! argument types are somewhat relaxed.
program compile(string source, object|void handler,
                int|void major, int|void minor,
                program|void target, object|void placeholder)
{
  return 8.1::compile(source, handler, major, minor, target, placeholder);
}

protected Mapping.ShadowedMapping compat_all_constants =
  Mapping.ShadowedMapping(predef::all_constants(),
			  ([
#if constant(_Debug.HAVE_DEBUG)
			    "_assembler_debug": _Debug.assembler_debug,
			    "_debug": _Debug.debug,
			    "_describe": _Debug.describe,
			    "_dump_backlog": _Debug.dump_backlog,
			    "_dump_program_tables": _Debug.dump_program_tables,
			    "_gc_set_watch": _Debug.gc_set_watch,
			    "_leak": _Debug.leak,
			    "_locate_references": _Debug.locate_references,
			    "_optimizer_debug": _Debug.optimizer_debug,
#endif /* _Debug.HAVE_DEBUG */
			    "_map_all_objects": _Debug.map_all_objects,
			    "_refs": _Debug.refs,
                            "_next": _Debug.next,
                            "_prev": _Debug.prev,
                            "Iterator": Iterator,
			    "get_iterator": get_iterator,
			    "hash": hash_8_0,
                            "next_object": _Debug.next_object,
                            "add_constant": add_constant,
			    "all_constants": all_constants,
                            "compile": compile,
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
