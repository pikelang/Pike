#pike __REAL_VERSION__

mapping cache=([]);

mixed `[](string s)
{
  if(!zero_type(cache[s])) return cache[s];
  mixed tmp=_static_modules[s];
  if(programp(tmp)) tmp=tmp();
  return cache[s]=tmp;
}

