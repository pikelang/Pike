#pike __REAL_VERSION__

// $Id$

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

static array(string) local_path;

mixed `[](string name) {
  // Make sure dot syntax Local.add_path works.
  if(name=="add_path" || name=="remove_path"){
    if(name=="add_path")
      return add_path;
    return remove_path;
  }

  // FIXME: Should .pike och .pmod files have priority?
  //        currently .pike files has it here, but .pmod in the
  //        resolver. // mikael
  
  foreach(local_path,string lp){
    program r=(program)combine_path(lp,name);
    if(r)
      return r;
    Stdio.Stat st;
    if(st=file_stat(lp+name+".pmod")){
      if(st->isdir)
        return master()->dirnode(lp+name+".pmod");
      return (object)(lp+name+".pmod");
    }
    catch{
      return (object)(lp+name+".so");
    };

    // This is not a proper handling of .so-files, it does not
    // handle when there are conflicting .so and .pmod files.

  }

  return UNDEFINED;
}

array(string) _indices() {
  array(string) tmp = ({ });
  foreach(local_path,string lp)
    tmp+=get_dir(lp);
  return Array.uniq(map(glob("*.pike",tmp)+glob("*.pmod",tmp)+glob("*.so",tmp),
             lambda(string in){ if(glob("*.so",in)) return in[..sizeof(in)-4];
             return in[..sizeof(in)-6]; }));
}

// _values intentionally not overloaded

static void create() {
  string tmp;
  local_path=({ });
  // FIXME $prefix/pike_modules
  // FIXME All this should be controllable from .pikerc, when such a file is implemented...
  add_path("/usr/local/pike_modules");
  add_path("/opt/pike_modules");
  add_path("/opt/share/pike_modules");
  add_path("/usr/local/share/pike_modules");

  if( (tmp=getenv("HOME")) || (tmp=getenv("USERPROFILE")) ) {
    tmp = (tmp[-1]=='/'?tmp:tmp+"/")+"pike_modules/";
    add_path(tmp);
  }

  if(tmp = getenv("PIKE_LOCAL_PATH") ) {
    array to_add=reverse(tmp/":"); // preserve order
    add_path( to_add[*] ); 
  }
}

//! This function prepends @[path] to the @[Local] module
//! searchpath.
//!
//! @param path
//!   The path to the directory to be added.
//!
//! @returns
//!   Returns 1 on success, otherwise 0.
int(0..1) add_path(string path){
  if(!file_stat(path))
    return 0;
  if(path[sizeof(path)-1]!='/')
    path+="/";
  remove_path(path); // Avoid duplicate entries.
  local_path=({path})+local_path;
  return 1;
}

//! This function removes @[path] from the @[Local] module
//! searchpath. If @[path] is not in the search path, this is
//! a noop.
//!
//! @param path
//!   The path to be removed.
//!
void remove_path(string path){
  if(local_path)
    local_path-=({ path });
}
