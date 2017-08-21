#pike __REAL_VERSION__

//! This module provides lazy resolving of
//! function. @code{Pike.Lazy.Standards.JSON.decode("{}");@} will not
//! resolve until the line is executed the first time. Note that this
//! destroys type information and delays resolver errors.

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

protected mixed `() = lookup;
