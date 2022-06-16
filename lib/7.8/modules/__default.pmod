// Compatibility namespace

#pike 7.9

//! Pike 7.8 compatibility.
//!
//! The symbols in this namespace will appear in
//! programs that use @tt{#pike 7.8@} or lower.

//! @decl inherit 8.0::

array|mapping|multiset|string map(array|mapping|multiset|string|object|program stuff, function(__unknown__, __unknown__...:mixed) f, mixed ... args)
{
  if(objectp(stuff))
  {
    catch {
      return 7.9::map(stuff, f, @args);
    };
    if( catch {
        object o = stuff;
        stuff = allocate(sizeof(o));
        for( int i; i<sizeof(stuff); i++ )
          stuff[i] = o[i];
      } )
      error("Bad argument 1 to map. Expected object that works in map.\n");
  }
  return 7.9::map(stuff, f, @args);
}

object master()
{
  return __REAL_VERSION__::master()->get_compat_master(7, 8);
}

protected Mapping.ShadowedMapping compat_all_constants =
  Mapping.ShadowedMapping(predef::all_constants(),
			  ([
			    "all_constants" : all_constants,
                            "map" : map,
			    "master" : master,
			  ]), 1);

mapping(string:mixed) all_constants()
{
  // Intentional lie in the return type.
  mixed x = compat_all_constants;
  return x;
}
