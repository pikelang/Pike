/* Compatibility module */

#pike 7.1

mapping FScache=set_weak_flag(([]),1);

class FS
{
  mixed old;

  array(int) file_stat(string s, void|int x)
    {
      if(object y=old(s, x))
	return (array)y;
      return 0;
    }

  void create(function o)
    {
      old=o;
      FScache[o]=file_stat;
    }
}

string _typeof(mixed x)
{
  return sprintf("%O",predef::_typeof(x));
}

mapping m_delete(mapping m, mixed x)
{
  predef::m_delete(m,x);
  return m;
}

int hash(string s, int|void f )
{
  if( f )
    return hash_7_0( s, f );
  return hash_7_0( s );
}

mapping(string:mixed) all_constants()
{
  mapping(string:mixed) ret=predef::all_constants()+([]);

  /* support overloading */
  ret->file_stat=FScache[ret->file_stat] || FS(ret->file_stat)->file_stat;

  /* does not support overloading (yet) */
  ret->all_constants=all_constants;
  ret->_typeof=_typeof;
  ret->m_delete=m_delete;
  ret->hash=hash_7_0;

  return ret;
}
