/* Compatibility module */

#pike 7.0

//! Pike 0.6 compatibility.
//!
//! The symbols in this namespace will appear in
//! programs that use @tt{#pike 0.6@} or lower.
//!
//! @seealso
//!   @[7.0::]

//! @decl inherit 7.0::

//! More lax types than in later versions.
array(mixed) aggregate(mixed ... args)
{
  return args;
}

mapping(string:mixed) all_constants()
{
  mapping(string:mixed) ret=predef::all_constants()+([]);

  foreach(indices(this_object()), string id) ret[id]=::`->(id);

  return ret;
}
