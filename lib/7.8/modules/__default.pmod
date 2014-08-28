// Compatibility namespace

#pike 7.9

//! Pike 7.8 compatibility.
//!
//! The symbols in this namespace will appear in
//! programs that use @tt{#pike 7.8@} or lower.

//! @decl inherit predef::

array|mapping|multiset|string map(array|mapping|multiset|string|object|program stuff, function(mixed,mixed...:mixed) f, mixed ... args)
{
  if(objectp(stuff))
  {
    if( catch { stuff = (array)stuff; } )
      if( catch { stuff = (mapping)stuff; } )
        if( catch { stuff = (multiset)stuff; } )
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

protected Mapping.ShadowedMapping compat_all_constants =
  Mapping.ShadowedMapping(predef::all_constants(),
			  ([
                            "map" : map,
			  ]), 1);

mapping(string:mixed) all_constants()
{
  // Intentional lie in the return type.
  mixed x = compat_all_constants;
  return x;
}
