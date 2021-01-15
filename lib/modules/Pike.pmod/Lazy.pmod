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

protected string path;

protected void create(void|string path)
{
  this::path = path;
}

protected mixed `[](string id)
{
  if(path)
    id = path + "." + id;
  return this_program(id);
}

protected mixed lookup(mixed ... args)
{
  function f = master()->resolv(path);
  if(!f) error("Failed to resolve %O\n");
  `() = f;
  return f(@args);
}

protected function(mixed...:mixed) `() = lookup;
