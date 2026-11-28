#pike __REAL_VERSION__
#pragma strict_types

//! Alias for @[m_delete()]
constant delete=m_delete;

constant Iterator = __builtin.mapping_iterator;

//! A mapping look-alike that overrides (ie shadows) another @[parent] mapping.
//!
//! The class implements most of the usual mapping operations, including
//! @[sizeof()] and @tt{foreach}. Iteration uses a snapshot of the merged
//! entries.
//!
//! @[m_delete()] removes a shadow entry if present, or the parent entry
//! otherwise. With modify_parent set to 2, deleting a shadow entry also
//! removes the corresponding parent entry.
//!
//! @note
//!   Use a mapping cast to obtain a mapping value from the view.
class ShadowedMapping(protected mapping|ShadowedMapping parent)
{
  protected mapping shadow = ([]);

  protected mapping|zero joined;
  protected mapping|zero parent_copy;

  protected int(0..2) modify_parent;

  //! @decl void create(mapping|ShadowedMapping parent, @
  //!                   mapping|void shadow, int(0..2)|void modify_parent)
  //!
  //! @param parent
  //!   Mapping to be shadowed.
  //! @param shadow
  //!   Initial shadow of @[parent].
  //! @param modify_parent
  //!   Modifications should be done to @[parent] rather than
  //!   to @[shadow].
  //!   @int
  //!     @value 0
  //!       Modifications should be done only to @[shadow].
  //!     @value 1
  //!       Entries that already present in @[shadow] can be modified
  //!       by later operations. Other modifications will be performed
  //!       in @[parent].
  //!     @value 2
  //!       All modifications will be performed in @[parent]. If the
  //!       entry to be modified is present in @[shadow], it will
  //!       be removed from it.
  //!   @endint
  protected void create(mapping|void shadow, int(0..2)|void modify_parent)
  {
    if (shadow) this::shadow = shadow + ([]);
    this::modify_parent = modify_parent;
  }

  // Updates the cached joined mapping if needed.
  protected void update_joined()
  {
    if (!joined || !equal(parent, parent_copy)) {
      joined = [mapping](parent + shadow);
      parent_copy = [mapping](parent + ([]));
    }
  }

  protected mixed `[](mixed ind)
  {
    mixed res = shadow[ind];
    if (!undefinedp(res)) return res;
    return parent[ind];
  }

  protected void `[]=(mixed ind, mixed val)
  {
    joined = 0;
    if (modify_parent) {
      if ((modify_parent == 1) && has_index(shadow, ind)) {
        shadow[ind] = val;
      } else {
        m_delete(shadow, ind);
        parent[ind] = val;
      }
    } else {
      shadow[ind] = val;
    }
  }

  protected mixed `->(string ind)
  {
    return `[](ind);
  }

  protected void `->=(string ind, mixed val)
  {
    `[]=(ind, val);
  }

  protected int(0..1) _equal(mixed other)
  {
    update_joined();
    return equal(other, joined);
  }

  protected mixed _m_delete(mixed ind, mixed|void expected)
  {
    mixed res;
    if (has_index(shadow, ind)) {
      res = m_delete(shadow, ind, expected);
      if (modify_parent == 2 && !has_index(shadow, ind)) {
        m_delete(parent, ind);
      }
    } else {
      res = m_delete(parent, ind, expected);
    }
    joined = 0;
    return res;
  }

  protected int _sizeof()
  {
    update_joined();
    return sizeof(joined);
  }

  protected Iterator _get_iterator()
  {
    update_joined();
    return Iterator(joined);
  }

  protected array(mixed) _indices()
  {
    update_joined();
    return indices(joined);
  }

  protected array(mixed) _values()
  {
    update_joined();
    return values(joined);
  }

  protected mixed `+(mapping|zero|this_program arg)
  {
    update_joined();
    return predef::`+(joined, arg);
  }

  protected mixed ``+(mapping|zero|this_program arg)
  {
    update_joined();
    return predef::`+(arg, joined);
  }

  protected mixed `-(mapping|zero|this_program arg)
  {
    update_joined();
    return predef::`-(joined, arg);
  }

  protected mixed ``-(mapping|zero|this_program arg)
  {
    update_joined();
    return predef::`-(arg, joined);
  }

  protected mixed `|(mixed arg)
  {
    update_joined();
    return predef::`|(joined, [mapping|object]arg);
  }

  protected mixed ``|(mixed arg)
  {
    update_joined();
    return predef::`|(arg, joined);
  }

  protected mixed `&(mixed arg)
  {
    update_joined();
    return predef::`&(joined, [array|mapping|multiset|object]arg);
  }

  protected mixed ``&(mixed arg)
  {
    update_joined();
    return predef::`&(arg, joined);
  }

  protected mixed `^(mixed arg)
  {
    update_joined();
    return predef::`^(joined, [mapping|object]arg);
  }

  protected mixed ``^(mixed arg)
  {
    update_joined();
    return predef::`^(arg, joined);
  }

  protected mixed cast(string type)
  {
    update_joined();
    switch(type) {
    case "mapping": return joined + ([]);
    case "array": return (array)joined;
    }
    return UNDEFINED;
  }

  protected mixed _search(mixed val)
  {
    update_joined();
    return search(joined, val);
  }

  protected string _sprintf(int c)
  {
    update_joined();
    return sprintf(sprintf("%%%c", c), joined);
  }
}
