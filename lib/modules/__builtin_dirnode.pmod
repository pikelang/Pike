#pike __REAL_VERSION__
#pragma strict_types

mapping(string:mixed) cache=([]);

mixed `[](string s)
{
  if(!zero_type(cache[s])) return cache[s];
  mixed tmp=_static_modules[s];
  if(programp(tmp)) tmp=([program]tmp)();
  return cache[s]=tmp;
}

