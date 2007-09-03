#pike __REAL_VERSION__

// $Id: Local.pmod,v 1.10 2007/09/03 13:05:08 grubba Exp $

//! @[Local] gives a local module namespace used for locally
//! installed pike modules. Modules are searched for in
//! the directory @tt{pike_modules@} which can be located in
//! the user's home directory or profile directory, or in any
//! of the system directories @tt{/opt/share, /usr/local/share,
//! /opt@} or @tt{/usr/local/@}. The user's home directory is
//! determined by examining the environment variable HOME, and
//! if that fails the environment variable USERPROFILE. If the 
//! environment variable PIKE_LOCAL_PATH is set, the paths specified
//! there will be searched first.
//! @seealso
//!   @[Local.add_path()], @[Local.remove_path()]
//!

inherit __joinnode;

static array(string) local_path;

static void create()
{
  ::create(({}));

  // FIXME $prefix/pike_modules
  // FIXME All this should be controllable from .pikerc, when such a file is implemented...

  add_path("/usr/local/pike_modules");
  add_path("/opt/pike_modules");
  add_path("/opt/share/pike_modules");
  add_path("/usr/local/share/pike_modules");

  string tmp;
  if( (tmp=[string]getenv("HOME")) || (tmp=[string]getenv("USERPROFILE")) ) {
    tmp = (tmp[-1]=='/'?tmp:tmp+"/")+"pike_modules/";
    add_path(tmp);
  }

  if(tmp = [string]getenv("PIKE_LOCAL_PATH") ) {
    array to_add=reverse(tmp/":"); // preserve order
    add_path( to_add[*] ); 
  }
}

//! @decl int(0..1) add_path(string path)
//!
//! This function prepends @[path] to the @[Local] module
//! searchpath.
//!
//! @param path
//!   The path to the directory to be added.
//!
//! @returns
//!   Returns 1 on success, otherwise 0.

//! This function removes @[path] from the @[Local] module
//! searchpath. If @[path] is not in the search path, this is
//! a noop.
//!
//! @param path
//!   The path to be removed.
//!
void remove_path(string path)
{
  rem_path(path);
}
