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

  int isfifo() { return (mode&0xF000)==0x1000; }
  int ischr()  { return (mode&0xF000)==0x2000; }
  int isdir()  { return (mode&0xF000)==0x4000; }
  int isblk()  { return (mode&0xF000)==0x6000; }
  int isreg()  { return (mode&0xF000)==0x8000; }
  int islnk()  { return (mode&0xF000)==0xa000; }
  int issock() { return (mode&0xF000)==0xc000; }
  int isdoor() { return (mode&0xF000)==0xd000; }

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

  void attach_statarray(array(int) a)
  {
    [mode, size, atime, mtime, ctime, uid, gid] = a;
    if(size<0)
      size = 0;
  }

  Stdio.File open(string mode)
  {
    return filesystem->open(fullpath, mode);
  }

  object /* Filesystem.Base */ cd()
  {
    if(isdir())
      return filesystem->cd(fullpath);
    // fallback to tar, etc

    // something like this? /jhs
    //  return Filesystem.Tar(fullpath, this_object());
    return 0;
  }

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

  string _sprintf()
  {
    return sprintf("Stat(/* %s */)", lsprint(1));
  }
}

class Base
{
  Base cd(string|void directory);
  string cwd();
  Base chroot(void|string directory);

  Stat stat(string file);

  array(string) get_dir(void|string directory, void|string|array glob);
  array(Stat) get_stats(void|string directory, void|string|array glob);

  Stdio.File open(string filename, string mode);

  // int access(string filename, string mode);

  int apply();

  void chmod(string filename, int|string mode);
  void chown(string filename, int|object owner, int|object group);

  int mkdir(string directory, void|int|string mode);
  int rm(string filename);

  array find(void|function(Stat:int) mask, mixed ...extra);

  // for simplicity in descendants
  static program this_class = object_program(this_object());
}

int parse_mode(int old, int|string mode)
{
  if(intp(mode)) return mode;
  error("string parsing of mode not implemented");
}

program get_filesystem(string what)
{
  // runtime!
  return master()->resolv("Filesystem")[what];
}

function `()(void|string path)
{
  return get_filesystem("System")(".")->cd(path||".") ||
	 error("Can't create filesystem on given path\n"),0;
}
