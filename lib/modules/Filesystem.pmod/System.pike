#pike __REAL_VERSION__

//! Implements an abstraction of the normal filesystem.

inherit Filesystem.Base;

// Some notes about NT code:
//   Paths are assumed to follow one of two different patterns:
//    1. /foo/bar/gazonk     indicates a path on the current drive
//    2. D:/foo/bar/gazonk   a path on the drive indicated by D.
//
//    It's possible to change the root from /foo/bar to d:/foo/bar/something,
//    but it's not possible to change from c:/foo/bar to d:/foo/bar
//
//  Generally speaing, paths will almost always follow the second
//  pattern, since getcwd() is used to get the initial value of the
//  'wd' variable unless a different path is specified.
//
static Filesystem.Base parent; // parent filesystem

static string root = ""; // Note: Can now include leading "/"
static string wd;        // never trailing "/"

//! @decl void create(void|string directory, void|string root, void|int fast, void|Filesystem.Base parent)
//! Instanciate a new object representing the filesystem.
//! @param directory
//! The directory (in the real filesystem) that should become
//! the root of the filesystemobject.
//! @param root
//! Internal
//! @param fast
//! Internal
//! @param parent
//! Internal
static void create(void|string directory,  // default: cwd
		   void|string _root,   // internal: root
		   void|int fast,       // internal: fast mode (no check)
		   void|Filesystem.Base _parent)
   				 // internal: parent filesystem
{
  if( _root )
  {
#ifdef __NT__
    _root = replace( _root, "\\", "/" );
#endif
    sscanf(reverse(_root), "%*[/]%s", root);
    root = reverse( root ); // do not remove leading '/':es.
  }

  if(!fast)
  {
    Stdio.Stat a;
    if(!directory || directory=="" || directory[0]!='/')
      directory = combine_path(getcwd(), directory||"");
#ifdef __NT__
    directory = replace( directory, "\\", "/" );
    string p;
    if( sizeof(root) )
    {
      if( sscanf( directory, "%s:/%s", p, directory ) )
      {
	if( sizeof( root ) )
	{
	  if( !has_value( root, ":/" ) )
	    root = p+":"+ (root[0] == '/' ?"":"/") + root;
	  else
	    error( "Cannot change directory to above"+root+"\n" );
	}
	else
	  root = p+":/";
      }
    }
#endif
    while( sizeof(directory) && directory[0] == '/' )
      directory = directory[1..];
    while( sizeof(directory) && directory[-1] == '/' )
      directory = directory[..sizeof(directory)-2];
#ifdef __NT__
    if( sizeof( directory ) != 2 || directory[1] != ':' )
#endif
      if(!(a = file_stat(combine_path("/",root,directory))) || !a->isdir)
	error("Not a directory\n");
  }
  while( sizeof(directory) && directory[0] == '/' )
    directory = directory[1..];
  while( sizeof(directory) && directory[-1] == '/' )
    directory = directory[..sizeof(directory)-2];
  wd = directory;
}

static string _sprintf(int t)
{
  return t=='O' && sprintf("%O(/* root=%O, wd=%O */)", this_program, root, wd);
}

Filesystem.Base cd(string directory)
{
  Filesystem.Stat st = stat(directory);
#ifdef __NT__
  directory = replace( directory, "\\", "/" );
#endif
  if(!st) return 0;
  if(st->isdir()) // stay
    return this_program(combine_path(wd, directory),
			root, 1, parent);
  return st->cd(); // try something else
}

Filesystem.Base cdup()
{
  return cd("..");
}

string cwd()
{
  return wd;
}

Filesystem.Base chroot(void|string directory)
{
  if(directory)
  {
    Filesystem.Base new = cd(directory);
    if(!new) return 0;
    return new->chroot();
  }
  return this_program("", combine_path("/",root,wd), 1, parent);
}

Filesystem.Stat stat(string file, int|void lstat)
{
   Stdio.Stat a;
#ifdef __NT__
   file = replace( file, "\\", "/" );
   while( sizeof(file) && file[-1] == '/' )
     file = file[..sizeof(file)-2];
#endif
   string full = combine_path(wd, file);
   if ( full!="" && full[0]=='/') full=full[1..];

   if((a = file_stat(combine_path("/",root,full), lstat)))
   {
     Filesystem.Stat s = Filesystem.Stat();
     s->fullpath = full;
     s->name = file;
     s->filesystem = this_object();
     s->attach_statobject(a);
     return s;
   }
   else
     return 0;
}

array(string) get_dir(void|string directory, void|string|array(string) globs)
{
#ifdef __NT__
  if(directory)
  {
    directory = replace( directory, "\\", "/" );
    while( sizeof(directory) && directory[-1] == '/' )
      directory = directory[..sizeof(directory)-2];
  }
#endif
  directory = directory ? combine_path(wd, directory) : wd;

  array(string) y = predef::get_dir(combine_path("/",root,directory));
  if(!globs)
    return y;
  else if(stringp(globs))
    return glob(globs, y);
  else
  {
    array(string) p = ({});
    foreach(globs, string g)
    {
      array(string) z;
      p += (z = glob(g, y));
      y -= z;
    }
    return p;
  }
}

array(Filesystem.Stat) get_stats(void|string directory,
				 void|string|array(string) globs)
{
  Filesystem.Base z = this_object();
#ifdef __NT__
  if(directory)
  {
    directory = replace( directory, "\\", "/" );
    while( sizeof(directory) && directory[-1] == '/' )
      directory = directory[..sizeof(directory)-2];
  }
#endif
  if(directory &&
     !(z = z->cd(directory)))
    return 0;

  array(string) a = z->get_dir("", globs);
  if(!a) return 0;

  return map(a, z->stat, 1)-({0});
}

Stdio.File open(string filename, string mode)
{
#ifdef __NT__
  filename = replace( filename, "\\", "/" );
#endif
  filename = combine_path(wd, filename);
  if ( filename!="" && filename[0]=='/') filename=filename[1..];
  
  Stdio.File f = Stdio.File();

  if( !f->open( combine_path("/",root,filename), mode) )
    return 0;
  return f;
}

// int access(string filename, string mode)
// {
//   return 1; // sure
// }

int rm(string filename)
{
#ifdef __NT__
  filename = replace( filename, "\\", "/" );
#endif
  filename = combine_path(wd, filename);
  return predef::rm(combine_path("/",root,filename));
}

void chmod(string filename, int|string mode)
{
#ifdef __NT__
  filename = replace( filename, "\\", "/" );
#endif
  filename = combine_path(wd, filename);
  if(stringp(mode))
  {
    Filesystem.Stat st = stat(filename); // call to self
    if(!st) return 0;
    mode = Filesystem.parse_mode(st->mode, mode);
  }
  predef::chmod(combine_path("/",root,filename), mode);
}

void chown(string filename, int|object owner, int|object group)
{
#if constant(chown)
#ifdef __NT__
  filename = replace( filename, "\\", "/" );
#endif
  if(objectp(group))
    error("user objects not supported (yet)\n");
  if(objectp(owner))
    error("user objects not supported (yet)\n");

  filename = combine_path(wd, filename);
  predef::chown(combine_path("/",root,wd), owner, group);
#else
  error("system does not have a chown"); // system does not have a chown()
#endif
}

array find(void|function(Filesystem.Stat:int) mask, mixed ... extra)
{
  array(Filesystem.Stat) res = ({});
  array(Filesystem.Stat) d = get_stats() || ({});
  array(Filesystem.Stat) r = filter(d, "isdir");

  if(mask)
    res += filter(d-r, mask, @extra);
  else
    res += d-r;

  foreach(r, Filesystem.Stat dir)
  {
    if(!mask || mask(dir, @extra))
      res += ({ dir });

    if(dir->name=="." || dir->name=="..")
      continue;
    res += dir->cd()->find(mask, @extra);
  }

  return res;
}
