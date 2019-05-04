#pike __REAL_VERSION__

//! This class works pretty much as a mapping except the order of the indices
//! is kept in the order they are added.
//!
//! @example
//! @code
//! OrderedMapping m1 = OrderedMapping("foo", 1, "bar", 2);
//! m1->gazonk = 3;
//!
//! foreach (m1; string key; int val) {
//!   write("# %s: %d\n", key, val);
//! }
//!
//! /*
//!   output:
//!     # foo: 1
//!     # bar: 2
//!     # gazonk: 3
//! */
//!
//! m_delete(m1, "gazonk");
//!
//! m1 += OrderedMapping("afoo", 3);
//!
//! foreach (m1; string key; int val) {
//!   write("# %s: %d\n", key, val);
//! }
//!
//! /*
//!   output:
//!     # foo: 1
//!     # bar: 2
//!     # afoo: 3
//! */
//! @endcode
//!
//! @fixme
//!  Implement subtraction operator


//! @ignore
protected array(string) __indices = ({});
protected array(mixed)  __values  = ({});
//! @endignore


//! @example
//! @code
//! ADT.OrderedMapping m1 = ADT.OrderedMapping("key1", "val1",
//!                                            "key2", "val2");
//! @endcode
//!
//! @throws
//!  An error is thrown if the number of arguments isn't even.
//!
//! @param args
//!  Odd arguments are the indices, even arguments the values.
//!  @expr{"index", "value", "index", "value", ... @}
protected void create(mixed ... args)
{
  if (args) {
    if (sizeof(args) % 2 != 0) {
      error("Need even number of arguments! ");
    }

    for (int i; i < sizeof(args); i += 2) {
      __indices += ({ args[i]   });
      __values  += ({ args[i+1] });
    }
  }
}


//! @example
//! @code
//! ADT.OrderedMapping m1 = ADT.OrderedMapping(({ "key1", "key2" }),
//!                                            ({ "val1", "val2" }));
//! @endcode
//!
//! @throws
//!  And error is thrown if the size of the @[keys] and @[values] doens't match.
//!
//! @param keys
//! @param values
variant protected void create(array keys, array values)
{
  if (keys && values) {
    if (sizeof(keys) != sizeof(values)) {
      error("The keys and values arrays must be of the same length! ");
    }

    __indices = keys;
    __values  = values;
  }
}


protected this_program `[]=(mixed key, mixed val)
{
  int pos;

  if ((pos = search(__indices, key)) > -1) {
    __values[pos] = val;
  }
  else {
    __indices += ({ key });
    __values  += ({ val });
  }

  return this_program;
}


protected mixed `[](mixed key)
{
  int pos;
  if ((pos = search(__indices, key)) > -1) {
    return __values[pos];
  }

  return UNDEFINED;
}


protected mixed `->=(mixed key, mixed val)
{
  return `[]=(key, val);
}


protected mixed `->(mixed key)
{
  return `[](key);
}


protected mixed `+(mixed what)
{
  array args = mk_new_args(__indices, __values);

  if (mappingp(what) ||
      (objectp(what) && Program.implements(what, this_program)))
  {
    if (sizeof(what)) {
      args += mk_new_args(indices(what), values(what));
    }
  }
  else if (arrayp(what)) {
    if (sizeof(what) % 2 != 0) {
      error("Can't add an array of uneven size! ");
    }

    args += what;
  }
  else {
    error("Can not add %O to %O\n", _typeof(what), object_program(this));
  }

  return this_program(@args);
}


//! Cast the object into various other types.
//!
//! @note
//!  This method can not be called on the object. A proper @expr{(cast)@} has
//!  to be done.
//!
//! @param how
//!  @string
//!   @value mapping
//!    Will return a @expr{mapping@}. This will of course break the
//!    "orderness" of this object's indices.
//!   @value array
//!    Will return an @expr{array(array)@} where the inner array
//!    has two values where the first is the index and the second the value.
//!   @value multiset
//!    Will return the indices as a @expr{multiset@}
//!   @value program
//!    Will return the @expr{program@} the object was instantiated from.
//!  @endstring
protected mapping|array|multiset|program cast(string how)
{
  switch (how)
  {
    case "mapping":
      return mkmapping(__indices, __values);

    case "array":
      array out = ({});
      for (int i; i < sizeof(__indices); i++) {
        out += ({ ({ __indices[i], __values[i] }) });
      }
      return out;

    case "multiset":
      return (multiset)__indices;

    case "program":
      return this_program;
  }

  error("Can't cast %O() to %q! ", object_program(this), how);
}


//! Creates an array of arrays where the inner array will have two indices,
//! where the first is the value from @[ind] and the second the value from
//! @[val].
//!
//! @param ind
//!  Array of indices
//! @param val
//!  Array of values
protected array mk_new_args(array ind, array val)
{
  array args = ({});

  for (int i; i < sizeof(ind); i++) {
    args += ({ ind[i], val[i] });
  }

  return args;
}


protected int _sizeof()
{
  return sizeof(__indices);
}


protected mixed _m_delete(string key)
{
  int pos;
  if ((pos = search(__indices, key)) > -1) {
    mixed val = __values[pos];
    __indices = __indices[..pos-1] + __indices[pos+1..];
    __values  = __values[..pos-1]  + __values[pos+1..];
    return val;
  }

  return UNDEFINED;
}


protected InternalIterator _get_iterator()
{
  return InternalIterator();
}


protected array _indices()
{
  return __indices;
}


protected array _values()
{
  return __values;
}


protected string _sprintf(int t)
{
  return sprintf("%O\n", (mapping) this);
}


protected class InternalIterator
{
  private int idx = 0;

  protected int _sizeof()
  {
    return sizeof(__indices);
  }


  bool next()
  {
    if (has_index(__values, idx+1)) {
      idx += 1;
      return true;
    }

    return false;
  }


  bool first()
  {
    idx = 0;
    return !!sizeof(__indices);
  }


  string index()
  {
    return __indices[idx];
  }


  mixed value()
  {
    return __values[idx];
  }


  protected this_program `+(int steps)
  {
    this_program x = this_program();
    x += idx + steps;
    return x;
  }


  protected this_program `-(int steps)
  {
    this_program x = this_program();
    x += idx - steps;
    return x;
  }


  protected this_program `+=(int steps)
  {
    idx += steps;
    return this;
  }


  protected bool `!()
  {
    return !has_index(__values, idx);
  }
}
