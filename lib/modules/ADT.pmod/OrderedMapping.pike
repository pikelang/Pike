#pike __REAL_VERSION__

//! This class works pretty much as a mapping except the order of the indices
//! is kept in the order they are added.  This class works equivalent
//! to the Map() class in Javascript.
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

//! Type for the indices of the mapping.
__generic__ IndexType;

//! Type for the values of the mapping.
__generic__ ValueType;

//! @ignore
private array(mapping(IndexType:ValueType)|array(IndexType)) __internal =
  ({([]), ({})});

#define __map      __internal[0]
#define __indices  __internal[1]
//! @endignore

inline private void addelm(IndexType key, ValueType value) {
  __indices += ({ key });
  __map[key] = value;
}

private void __addelms(IndexType|ValueType ... args) {
  if (sizeof(args) % 2 != 0)
    error("Need even number of arguments! ");
  for (int i; i < sizeof(args); i += 2) {
    IndexType key = args[i];
    ValueType val = args[i+1];
    if (zero_type(__map[key]))
      __indices -= ({ key });
    addelm(key, val);
  }
}

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
protected void create(IndexType|ValueType ... args) {
  __addelms(@args);
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
variant protected void create(array(IndexType) keys, array(ValueType) values) {
  if (keys && values) {
    if (sizeof(keys) != sizeof(values))
      error("The keys and values arrays must be of the same length! ");
    __indices = keys;
    __map = mkmapping(keys, values);
    if (sizeof(keys) != sizeof(__map))
      error("Duplicate keys detected.");
  }
}

variant protected void create(this_program(<IndexType, ValueType>) old) {
  __indices = old->__indices;
  __map = old->__map + ([]);
}

protected program(this_program)|array|function `[]=(IndexType|string(7bit) key,
                                                    mixed val) {
  switch (key) {
    case "__internal": return __internal;
    case "__addelms": return __addelms;
  }
  if (zero_type(__map[key]))
    addelm(key, val);
  else
    __map[key] = val;
  return this_program;
}

private mapping switchblade = ([
  "__internal": __internal,
  "__addelms": __addelms,
 ]);

inline protected ValueType|array|function `[](mixed key) {
  return switchblade[key] || __map[key];
}

protected program(this_program)|array|function `->=(mixed key, mixed val) {
  return `[]=(key, val);
}

protected ValueType|array|function `->(mixed key) {
  return `[](key);
}

protected this_program `+(mapping|this_program|array what) {
  this_program copy = this_program(this);
  if (mappingp(what)) {
    copy->__map += what;
    copy->__indices += indices(what - __map);
  } else if (objectp(what) &&
             Program.implements(object_program(what), this_program)) {
    copy->__map += what->__map;
    copy->__indices += what->__indices - __indices;
  } else if (arrayp(what))
    copy->__addelms(what);
  else
    error("Can not add %O to %O\n", _typeof(what), object_program(this));
  return copy;
}

protected this_program `-(this_program|mapping|multiset|array what) {
  this_program copy = this_program(this);
  if (mappingp(what) || multisetp(what)) {
    copy->__map -= what;
    copy->__indices -= indices(what);
  } else if (objectp(what) &&
             Program.implements(object_program(what), this_program)) {
    copy->__map -= what->__map;
    copy->__indices -= what->__indices;
  } else if (arrayp(what)) {
    mapping cmap = copy->__map;
    for (int i; i < sizeof(what); i += 2) {
      mixed key = what[i];
      m_delete(cmap, key);
      copy->__indices -= ({ key });
    }
  } else
    error("Can not substract %O from %O\n", _typeof(what), object_program(this));
  return copy;
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
protected mapping|array|multiset|program cast(string how) {
  switch (how) {
    case "mapping":
      return __map + ([]);

    case "array":
      array out = ({});
      for (int i = 0; i < sizeof(__indices); i++) {
        mixed idx = __indices[i];
        out += ({ ({ idx, __map[idx] }) });
      }
      return out;

    case "multiset":
      return (multiset)__indices;

    case "program":
      return this_program;
  }

  error("Can't cast %O() to %q! ", object_program(this), how);
}

protected int _sizeof() {
  return sizeof(__indices);
}

protected object(ValueType)|zero _m_delete(IndexType key) {
  __indices -= ({ key });
  return m_delete(__map, key);
}

protected array(IndexType) _indices() {
  return __indices;
}

protected array(ValueType) _values() {
  array out = ({});
  for (int i = 0; i < sizeof(__indices); )
    out += ({ __map[__indices[i++]] });
  return out;
}

protected class InternalIterator (< IndexType = IndexType, ValueType = ValueType >) {
  private int idx = -1;

  protected int _sizeof() {
    return sizeof(__indices);
  }

  protected int _iterator_next() {
    if (++idx < sizeof(__indices)) {
      return idx;
    }
    idx = -1;
    return UNDEFINED;
  }


  bool first() {
    idx = -1;
    return !!sizeof(__indices);
  }


  protected object(IndexType)|zero _iterator_index() {
    return (idx >= 0) ? __indices[idx] : UNDEFINED;
  }


  protected object(ValueType)|zero _iterator_value() {
    return (idx >= 0) ? __map[__indices[idx]] : UNDEFINED;
  }


  protected this_program(<IndexType, ValueType>) `+=(int steps) {
    idx += steps;
    if (idx >= sizeof(__indices)) {
      idx = sizeof(__indices) - 1;
    }
    if (idx < -1) idx = -1;
    return this;
  }
}

protected InternalIterator _get_iterator() {
  return InternalIterator();
}

protected string _sprintf(int t) {
  string ret = sprintf("([ /* %d elements */\n", sizeof(this));
  foreach (this; IndexType key; ValueType value)
    ret += sprintf("  %O: %O,\n", key, value);
  return ret + "])";
}
