#pike __REAL_VERSION__
#pragma strict_types

//! Alias for @[m_delete()]
constant delete=m_delete;

constant Iterator = __builtin.mapping_iterator;

//! A mapping look-alike that overrides (ie shadows) another @[parent] mapping.
//!
//! The class implements most of the usual mapping operations.
class ShadowedMapping(static mapping|ShadowedMapping parent)
{
  static mapping shadow = ([]);

  static mapping joined;
  static int parent_generation = -1;

  static int generation;
  static int dirty = 1;

  // Updates the cached joined mapping if needed.
  static void update_joined()
  {
    if (!joined || (parent_generation != m_generation(parent))) {
      joined = [mapping](parent + shadow);
      parent_generation = m_generation(parent);
      dirty = 1;
    }
  }

  static mixed `[](mixed ind)
  {
    mixed res = shadow[ind];
    if (!zero_type(res)) return res;
    return parent[ind];
  }

  static void `[]=(mixed ind, mixed val)
  {
    joined = 0;
    dirty = 1;
    shadow[ind] = val;
  }

  static mixed `->(string ind)
  {
    return `[](ind);
  }

  static void `->=(string ind, mixed val)
  {
    `[]=(ind, val);
  }

  static int _m_generation()
  {
    if (dirty || (parent_generation != m_generation(parent))) {
      if (!dirty && (parent_generation != m_generation(parent)))
	update_joined();
      generation++;
      dirty = 0;
    }
    return generation;
  }

  static mixed _m_delete(mixed ind)
  {
    mixed res = m_delete(shadow, ind);
    if (zero_type(res)) {
      res = m_delete(parent, ind);
    }
    return res;
  }

  static array(mixed) _indices()
  {
    update_joined();
    return indices(joined);
  }

  static array(mixed) _values()
  {
    update_joined();
    return values(joined);
  }

  static mixed `+(mixed ... args)
  {
    update_joined();
    return predef::`+(joined, @args);
  }

  static mixed ``+(mixed ... args)
  {
    update_joined();
    return predef::`+(@args, joined);
  }

  static mixed `-(mixed ... args)
  {
    update_joined();
    return predef::`-(joined, @args);
  }

  static mixed ``-(mixed ... args)
  {
    update_joined();
    return predef::`-(@args, joined);
  }

  static mixed `|(mixed ... args)
  {
    update_joined();
    return predef::`|(joined, @args);
  }

  static mixed ``|(mixed ... args)
  {
    update_joined();
    return predef::`|(@args, joined);
  }

  static mixed `&(mixed ... args)
  {
    update_joined();
    return predef::`&(joined, @args);
  }

  static mixed ``&(mixed ... args)
  {
    update_joined();
    return predef::`&(@args, joined);
  }

  static mixed `^(mixed ... args)
  {
    update_joined();
    return predef::`^(joined, @args);
  }

  static mixed ``^(mixed ... args)
  {
    update_joined();
    return predef::`^(@args, joined);
  }

  static mixed cast(string type)
  {
    update_joined();
    switch(type) {
    case "mapping": return joined + ([]);
    case "array": return (array)joined;
    }
    return joined + ([]);
  }

  static mixed _search(mixed val)
  {
    update_joined();
    return search(joined, val);
  }

  static string _sprintf(int c)
  {
    update_joined();
    return sprintf(sprintf("%%%c", c), joined);
  }
}
