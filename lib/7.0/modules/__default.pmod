// Compatibility namespace
// $Id$

#pike 7.1

//! Pike 7.0 compatibility.
//!
//! The symbols in this namespace will appear in
//! programs that use @tt{#pike 7.0@} or lower.
//!
//! @seealso
//!   @[7.2::]

//! @decl inherit 7.2::

//! @decl array(int) file_stat(string path, void|int symlink)
//!   Stat a file (Pike 7.0 compatibility).
//!
//! @returns
//!   Returns an array with the following content:
//!   @array
//!     @elem int mode
//!       Access mode.
//!     @elem int size
//!       Size in bytes, or filetype indicator.
//!     @elem int atime
//!       Last access time.
//!     @elem int mtime
//!       Last modification time.
//!     @elem int ctime
//!       Last change time.
//!     @elem int uid
//!       User id.
//!     @elem int gid
//!       Group id.
//!   @endarray
//!
//!   See @[predef::file_stat()] for a full description of the array.
//!
//! @seealso
//!   @[predef::file_stat()]

static mapping(function:object(FS)) FScache = set_weak_flag(([]),1);

static class FS
{
  static function old;

  array(int) file_stat(string path, void|int symlink)
  {
    if(mixed y=old(path, symlink))
      return (array)y;
    return 0;
  }

  static void create(function o)
  {
    old=o;
    FScache[o]=file_stat;
  }
}

//! Get the runtime type of a value (Pike 7.0 compatibility).
//!
//! @returns
//!   Returns the type of @[x] as a string.
//!
//! @seealso
//!   @[predef::_typeof()]
string _typeof(mixed x)
{
  return sprintf("%O",predef::_typeof(x));
}

//! Delete an entry @[x] from a mapping @[m] (Pike 7.0 compatibility).
//!
//! @returns
//!   Returns @[m].
//!
//! @seealso
//!   @[predef::m_delete()]
mapping m_delete(mapping m, mixed x)
{
  predef::m_delete(m,x);
  return m;
}

//! @decl int hash(string s, int|void f )
//!   Calculate a hash of a string (Pike 7.0 compatibility).
//!
//!   This function is now available as @[predef::hash_7_0()].
//!
//! @seealso
//!   @[predef::hash_7_0()], @[predef::hash()]

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
