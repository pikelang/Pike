/* Compatibility module */

#pike 7.0

//! @decl inherit 7.0::

/* compatibility: this destroys too strict type checking */
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
