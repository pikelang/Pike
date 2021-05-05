#pike __REAL_VERSION__

//! This module provides lazy resolving of functions.
//!
//! @example
//!   The expression @expr{Pike.Lazy.Standards.JSON.decode@} will
//!   evaluate to @[Standards.JSON.decode], but the resolution
//!   (and associated compilation) of @[Standards.JSON] will be
//!   delayed until the first time that the expression is evaluated.
//!
//! @note
//!   Note that this destroys type information and delays resolver errors.
//!
//! Typical use is to break circular compilation dependencies between modules.

protected mapping(string: LazyValue) lazy_values =
  set_weak_flag(([]), Pike.WEAK_VALUES);

protected mixed `[](string id)
{
  mixed res = lazy_values[id];
  if (!undefinedp(res)) return res;
  res = LazyValue(id);
  if (!undefinedp(lazy_values[id])) return lazy_values[id];	// Race
  return lazy_values[id] = res;
}

protected class LazyValue(string path)
{
  protected mixed `[](string id)
  {
    return global::this[path + "." + id];
  }

  protected mixed lookup(mixed ... args)
  {
    function f = master()->resolv(path);
    if(!f) error("Failed to resolve %O\n", path);
    lazy_values[path] = `() = f;
    return f(@args);
  }

  protected function(mixed...:mixed) `() = lookup;

  protected string _sprintf(int c)
  {
    if (c == 'O') {
      return sprintf("%O(%O)", this_program, path);
    }
    return UNDEFINED;
  }
}
