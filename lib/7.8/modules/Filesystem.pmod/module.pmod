#pike 7.8

#pragma no_deprecation_warnings

//! Describes the stat of a file
class Stat
{
  int mode;
  int size;
  int uid, gid;
  string uname, gname;
  int atime, mtime, ctime;

  string fullpath;
  string name;
  object /* Filesystem.Base */ filesystem;

  //! Is the file a FIFO?
  //!
  //! @returns
  //! 1 if the file is of a specific type
  //! 0 if the file is not.
  //! @seealso
  //! [set_type]
  int(0..1) isfifo() { return (mode&0xF000)==0x1000; }

  //! Is the file a character device?
  //!
  //! @returns
  //! 1 if the file is of a specific type
  //! 0 if the file is not.
  //! @seealso
  //! [set_type]
  int(0..1) ischr()  { return (mode&0xF000)==0x2000; }

  //! Is the file (?) a directory?
  //!
  //! @returns
  //! 1 if the file is of a specific type
  //! 0 if the file is not.
  //! @seealso
  //! [set_type]
  int(0..1) isdir()  { return (mode&0xF000)==0x4000; }

  //! Is the file a block device?
  //!
  //! @returns
  //! 1 if the file is of a specific type
  //! 0 if the file is not.
  //! @seealso
  //! [set_type]
  int(0..1) isblk()  { return (mode&0xF000)==0x6000; }

  //! Is the file a regular file?
  //!
  //! @returns
  //! 1 if the file is of a specific type
  //! 0 if the file is not.
  //! @seealso
  //! [set_type]
  int(0..1) isreg()  { return (mode&0xF000)==0x8000; }

  //! Is the file a link to some other file or directory?
  //!
  //! @returns
  //! 1 if the file is of a specific type
  //! 0 if the file is not.
  //! @seealso
  //! [set_type]
  int(0..1) islnk()  { return (mode&0xF000)==0xa000; }

  //! Is the file a socket?
  //!
  //! @returns
  //! 1 if the file is of a specific type
  //! 0 if the file is not.
  //! @seealso
  //! [set_type]
  int(0..1) issock() { return (mode&0xF000)==0xc000; }

  //! @xml{<fixme>Document this function.</fixme>@}
  //!
  //! @returns
  //! 1 if the file is of a specific type
  //! 0 if the file is not.
  //! @seealso
  //! [set_type]
  int(0..1) isdoor() { return (mode&0xF000)==0xd000; }

  //! Set a type for the stat-object.
  //! @note
  //! This call doesnot change the filetype in the underlaying filesystem.
  //! @param x
  //! Type to set. Type is one of the following:
  //! @ul
  //! @item fifo
  //! @item chr
  //! @item dir
  //! @item blk
  //! @item reg
  //! @item lnk
  //! @item sock
  //! @item door
  //! @endul
  //! @seealso
  //! [isfifo], [ischr], [isdir], [isblk], [isreg], [islnk], [issock], [isdoor]
  void set_type(string x)
  {
    switch (x)
    {
    case "fifo": mode = (mode&~0xF000)|0x1000; break;
    case "chr":  mode = (mode&~0xF000)|0x2000; break;
    case "dir":  mode = (mode&~0xF000)|0x4000; break;
    case "blk":  mode = (mode&~0xF000)|0x6000; break;
    case "reg":  mode = (mode&~0xF000)|0x8000; break;
    case "lnk":  mode = (mode&~0xF000)|0xa000; break;
    case "sock": mode = (mode&~0xF000)|0xc000; break;
    case "door": mode = (mode&~0xF000)|0xd000; break;
    default: error("illegal type\n");
    }
  }

  //! Fills the stat-object with data from a Stdio.File.stat() call.
  void attach_statarray(array(int) a)
  {
    [mode, size, atime, mtime, ctime, uid, gid] = a;
    if(size<0)
      size = 0;
  }

// fixme
  void attach_statobject(object a)
  {
     attach_statarray( (array) a );
  }

  //! Open the stated file within the filesystem
  //! @returns
  //! a [Stdio.File] object
  //! @seealso
  //! [Stdio.File]
  Stdio.File open(string mode)
  {
    return filesystem->open(fullpath, mode);
  }

  //! Change to the stated directory.
  //! @returns
  //! the directory if the stated object was a directory, 0 otherwise.
  object /* Filesystem.Base */ cd()
  {
    if(isdir())
      return filesystem->cd(fullpath);
    // fallback to tar, etc

    // something like this? /jhs
    //  return Filesystem.Tar(fullpath, this);
    return 0;
  }

  //! Returns the date of the stated object as cleartext.
  string nice_date()
  {
    mapping tm = localtime(mtime);
    if(time()-86400*365>mtime)
      return sprintf("%2d %3s  %04d", tm->mday,
		     ({"Jan","Feb","Mar","Apr","May","Jun",
		       "Jul","Aug","Sep","Oct","Nov","Dec"})[tm->mon],
		     tm->year+1900);
    else
      return sprintf("%2d %3s %2d:%02d", tm->mday,
		     ({"Jan","Feb","Mar","Apr","May","Jun",
		       "Jul","Aug","Sep","Oct","Nov","Dec"})[tm->mon],
		     tm->hour, tm->min);
  }

  string lsprint(void|int path) // mostly debug purpose
  {
    return sprintf("%c%c%c%c%c%c%c%c%c%c%4s %-9s%-9s%8d %s %s",
		   "?fc?d?b?-?l?sD??"[(mode&0xf000)>>12],
		   "-r"[!!(mode&256)],
		   "-w"[!!(mode&128)],
		   "-x"[!!(mode&64)],
		   "-r"[!!(mode&32)],
		   "-w"[!!(mode&16)],
		   "-x"[!!(mode&8)],
		   "-r"[!!(mode&4)],
		   "-w"[!!(mode&2)],
		   "-x"[!!(mode&1)],
		   "", // blocks
		   uname||(string)uid, // no lookup
		   gname||(string)gid, // no lookup
		   size,
		   nice_date(),
		   path?fullpath:name);
  }

  string _sprintf(int t)
  {
    return t=='O' && sprintf("Stat(/* %s */)", lsprint(1));
  }
}

//! Baseclass that can be extended to create new filesystems.
//! Is used by the Tar and System filesystem classes.
class Base
{
  //! Change directory within the filesystem.
  //! Returns a new filesystem object with the given directory as cwd.
  Base cd(string|void directory);

  //! Returns the current working directory within the filesystem.
  string cwd();

  //! Change the root of the filesystem.
  Base chroot(void|string directory);

  //! Return a stat-object for a file or a directory within the
  //! filesystem.
  Stat stat(string file, int|void lstat);

  //! Returns an array of all files and directories within a given
  //! directory.
  //! @param directory
  //! Directory where the search should be made within the filesystem.
  //! CWD is assumed if none (or 0) is given.
  //! @param glob
  //! Return only files and dirs matching the glob (if given).
  //! @seealso
  //! [get_stats]
  array(string) get_dir(void|string directory, void|string|array glob);

  //! Returns stat-objects for the files and directories matching the
  //! given glob within the given directory.
  //! @seealso
  //! [get_dir]
  array(Stat) get_stats(void|string directory, void|string|array glob);

  //! Open a file within the filesystem
  //! @returns
  //! A Stdio.File object.
  Stdio.File open(string filename, string mode);

  // int access(string filename, string mode);

  //! @xml{<fixme>Document this function</fixme>@}
  int apply();

  //! Change mode of a file or directory.
  void chmod(string filename, int|string mode);

  //! Change ownership of the file or directory
  void chown(string filename, int|object owner, int|object group);

  //! Create a new directory
  int mkdir(string directory, void|int|string mode);

  //! Remove a file from the filesystem.
  int rm(string filename);

  //! @xml{<fixme>Document this function</fixme>@}
  array find(void|function(Stat:int) mask, mixed ...extra);
}

  //! @xml{<fixme>Document this function</fixme>@}
int parse_mode(int old, int|string mode)
{
  if(intp(mode)) return mode;
  error("string parsing of mode not implemented");
}

  //! @xml{<fixme>Document this function</fixme>@}
program get_filesystem(string what)
{
  // runtime!
  return master()->resolv("Filesystem")[what];
}

  //! @xml{<fixme>Document this function</fixme>@}
function `()(void|string path)
{
  return get_filesystem("System")(".")->cd(path||".") ||
	 error("Can't create filesystem on given path\n"),0;
}

//! Iterator object that traverses a directory tree and returns
//! files as values and paths as indices. Example that uses the
//! iterator to create a really simple sort of make:
//! @example
//!   object i=Filesystem.Traversion(".");
//!   foreach(i; string dir; string file) {
//!   	if(!has_suffix(file, ".c")) continue;
//!     file = dir+file;
//!     string ofile = file;
//!   	ofile[-1]='o';
//!   	object s=file_stat(ofile);
//!   	if(s && i->stat()->mtime<s->mtime) continue;
//!   	// compile file
//!   }
class Traversion {
  string path;
  array(string) files;
  object current;
  int(0..) pos;
  int(0..1) symlink;
  int(0..1) ignore_errors;
  function(array:array) sort_fun;
  constant is_traversion = 1;

  //! Returns the current progress of the traversion as a value
  //! between 0.0 and 1.0. Note that this value isn't based on the
  //! number of files, but the directory structure.
  float progress(void|float share) {
    if(!share) share=1.0;
    float sub = 0.0;
    if(current && current->is_traversion)
      sub = current->progress(share/sizeof(files));
    return share/sizeof(files)*(pos+1) + sub;
  }

  //! @decl void create(string path, void|int(0..1) symlink, void|int(0..1) ignore_errors, void|function(array:array) sort_fun)
  //! @param path
  //! The root path from which to traverse.
  //! @param symlink
  //! Don't traverse symlink directories.
  //! @param ignore_errors
  //! Ignore directories that can not be accessed.
  //! @param sort_fun
  //! Sort function to be applied to directory entries before
  //! traversing. Can also be a filter function.
  void create(string _path, void|int(0..1) _symlink, void|int(0..1) _ignore_errors, void|function(array:array) _sort_fun) {
    path = _path;
    if(path[-1]!='/') path+="/";
    files = get_dir(path);
    sort_fun = _sort_fun;
    if(sort_fun)
      files = sort_fun(files);
    symlink = _symlink;
    ignore_errors = _ignore_errors;
    if(!arrayp(files))
      if(ignore_errors)
        files = ({});
      else
        error( "Failed to access %s\n", path );
    if(sizeof(files)) set_current();
  }

  protected void set_current() {
    current = file_stat(path + files[pos]);
    if(!current) return;
    if(!current->isdir) return;
    if(symlink && file_stat(path + files[pos],1)->islnk) {
      pos++;
      if(pos < sizeof(files))
	set_current();
      return;
    }

    current = Traversion(path + files[pos], symlink, ignore_errors, sort_fun);
  }

  int `!() {
    if( pos >= sizeof(files) ) return 1;
    return 0;
  }

  //! Returns the stat for the current index-value-pair.
  Stdio.Stat stat() {
    if(!current) return 0;
    if(current->is_traversion)
      return current->stat();
    return current;
  }

  int add(int steps) {
    if(current && current->is_traversion)
      steps = current->add(steps);
    if(!steps) return 0;
    pos += steps;
    if( pos >= sizeof(files) )
      return pos - sizeof(files) + 1;
    set_current();
    return 0;
  }

  void `+=(int steps) {
    if (steps < 0) error ("Cannot step backwards.\n");
    add(steps);
  }

  string index() {
    if(current && current->is_traversion)
      return current->index();
    if( pos >= sizeof(files) ) return UNDEFINED;
    return path;
  }

  string value() {
    if(current && current->is_traversion)
      return current->value();
    if( pos >= sizeof(files) ) return UNDEFINED;
    return files[pos];
  }
}
