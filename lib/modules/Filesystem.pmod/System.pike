inherit Filesystem.Base;

static Filesystem.Base parent; // parent filesystem

static string root=""; // no "/" in beginning
static string wd; // always "/", no at the end

void create(void|string directory,  // default: cwd
	    void|string _root,   // internal: root
	    void|int fast,       // internal: fast mode (no check)
	    void|object(Filesystem.Base) _parent) 
   				 // internal: parent filesystem
{
   if (_root)
   {
      root=sscanf("%*[/]%s",_root)||_root;
      while (root!="" && (root[-1]=='/' || root[-1]=='\\'))
	 root=root[..strlen(root)-2];
   }

   if (!fast)
   {
      array a;

      if (!directory || directory=="" || directory[0]!='/')
	 directory=combine_path(getcwd(),directory||"");

      if (!(a=file_stat(root+directory)) ||
	  !((a[0]&0xF000)==0x4000))
	 error("Not a directory\n");
   }

   wd=directory;

   while (wd!="" && (wd[-1]=='/' || wd[-1]=='\\'))
      wd=wd[..strlen(wd)-2];
   if (wd=="") wd="/";
}

Filesystem.Base cd(string directory)
{
   Filesystem.Stat st=stat(directory);
   if (!st) return 0;
   if (st->isdir()) // stay
      return this_class(combine_path(wd,directory),
			root,1,parent);
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
   if (directory)
   {
      Filesystem.Base new=cd(directory);
      if (!new) return 0;
      return new->chroot();
   }
   return this_class("/",root+wd,1,parent);
}

Filesystem.Stat stat(string file)
{
   array a;
   string full=combine_path(wd,file);

   if ((a=file_stat(root+full)))
   {
      Filesystem.Stat s=Filesystem.Stat();
      s->fullpath=full;
      s->name=file;
      s->filesystem=this_object();
      s->attach_statarray(a);
      return s;
   }
   else
      return 0;
}

array(string) get_dir(void|string directory,void|string|array globz)
{
   directory=directory?combine_path(wd,directory):wd;

   array y=predef::get_dir(root+directory);
   if (!globz) return y;
   else if (stringp(globz)) return glob(globz,y);
   else 
   {
      array p=({});
      foreach (globz, string g)
      {
	 array z;
	 p+=(z=glob(g,y));
	 y-=z;
      }
      return p;
   }
}

array(Filesystem.Stat) get_stats(void|string directory,void|string|array glob)
{
   Filesystem.Base z=this_object();
   if (directory &&
       !(z=z->cd(directory)))
      return 0;

   array a=z->get_dir("",glob);
   if (!a) return 0;

   return Array.map(a,z->stat)-({0});
}

Stdio.File open(string filename,string mode)
{
   filename=combine_path(wd,filename);
   Stdio.File f=Stdio.File();
   
   if (!f->open(root+filename,mode))
      return 0;
   return f;
}

// int access(string filename,string mode)
// {
//    return 1; // sure
// }

int rm(string filename)
{
   filename=combine_path(wd,filename);
   return predef::rm(root+filename);
}

void chmod(string filename,int|string mode)
{
   filename=combine_path(wd,filename);
   if (stringp(mode))
   {
      Filesystem.Stat st=stat(filename); // call to self
      if (!st) return 0;
      mode=Filesystem.parse_mode(st->mode,mode);
   }
   predef::chmod(root+filename,mode);
}

void chown(string filename,int|object owner,int|object group)
{
#if constant(chown)
   if (objectp(group))
      error("user objects not supported (yet)\n");
   if (objectp(owner))
      error("user objects not supported (yet)\n");

   filename=combine_path(wd,filename);
   predef::chown(root+wd,owner,group);
#else
   error("system does not have a chown"); // system does not have a chown()
#endif
}

array find(void|function(Filesystem.Stat:int) mask, mixed ...extra)
{
   array res=({});
   array d,r;

   d=get_stats()||({});
   r=Array.filter(d,"isdir");
   if (mask) res+=Array.filter(d-r,mask,@extra); 
   else res+=d-r;
   
   foreach (r,Filesystem.Base dir) 
   {
      if (!mask) res+=({dir});
      else if (mask(dir,@extra)) res+=({dir});

      if (dir->name=="." || dir->name=="..") 
	 continue;
      res+=dir->cd()->find(mask,@extra);
   }
   
   return res;
}
