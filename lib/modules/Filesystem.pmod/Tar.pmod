

class _Tar  // filesystem
{
   object fd;
   string filename;

   class Record
   {
      inherit Filesystem.Stat;

      constant RECORDSIZE=512;
      constant NAMSIZ=100;
      constant TUNMLEN=32;
      constant TGNMLEN=32;
      constant SPARSE_EXT_HDR=21;
      constant SPARSE_IN_HDR=4;

      string arch_name;

      int linkflag;
      string arch_linkname;
      string magic;
      int devmajor;
      int devminor;
      int chksum;

      int pos;
      int pseudo;

      void create(void|string s, void|int _pos)
      {
	 if (!s) 
	 {
	    pseudo=1;
	    return;
	 }

	 pos=_pos;
	 array a=array_sscanf(s,"%"+((string)NAMSIZ)+"s%8s%8s%8s%12s%12s%8s" 
			      "%c%"+((string)NAMSIZ)+"s%8s"+
			      "%"+((string)TUNMLEN)+"s"+
			      "%"+((string)TGNMLEN)+"s%8s%8s");
	 sscanf(a[0],"%s%*[\0]",arch_name);
	 sscanf(a[1],"%o",mode);
	 sscanf(a[2],"%o",uid);
	 sscanf(a[3],"%o",gid);
	 sscanf(a[4],"%o",size);
	 sscanf(a[5],"%o",mtime);
	 sscanf(a[6],"%o",chksum);
	 linkflag=a[7];
	 sscanf(a[8],"%s%*[\0]",arch_linkname);
	 sscanf(a[9],"%s%*[\0]",magic);

	 if (magic=="ustar  ")
	 {
	    sscanf(a[10],"%s\0",uname);
	    sscanf(a[11],"%s\0",gname);
	 }
	 else
	    uname=gname=0;

	 sscanf(a[12],"%o",devmajor);
	 sscanf(a[13],"%o",devminor);

	 fullpath="/"+arch_name;
	 name=(fullpath/"/")[-1];
	 atime=ctime=mtime;

	 set_type( ([0:"reg",
		     '0':"reg",
		     '1':0, // hard link
		     '2':"lnk",
		     '3':"chr",
		     '4':"blk",
		     '5':"dir",
		     '6':"fifo",
		     '7':0 // contigous
	 ])[linkflag] || "reg" );
      }
   };

   array entries=({});
   array filenames;
   mapping filename_to_entry;

   void mkdirnode(string what,Record from, object parent)
   {
      Record r=Record();

      if (what=="") what="/";

      r->fullpath=what;
      r->name=(what/"/")[-1];

      r->mode=0755|((from->mode&020)?020:0)|((from->mode&02)?02:0);
      r->set_type("dir");
      r->uid=0;
      r->gid=0;
      r->size=0;
      r->atime=r->ctime=r->mtime=from->mtime;
      r->filesystem=parent;

      filename_to_entry[what]=r;
   }

   void create(Stdio.File fd,string filename,object parent)
   {
      // read all entries
    
      int pos=0; // fd is at position 0 here
      for (;;)
      {
	 Record r;
	 string s=fd->read(512);

	 if (s=="" || strlen(s)<512 ||
	     sscanf(s,"%*[\0]%*2s")==1) break;

	 r=Record(s,pos+512);
	 r->filesystem=parent;

	 if (r->arch_name!="")  // valid file?
	    entries+=({r});

	 pos=pos+512+r->size;
	 if (pos%512) pos+=512-(pos%512);
	 fd->seek(pos);
      }

      filename_to_entry=mkmapping(entries->fullpath,entries);

      // create missing dirnodes

      array last=({});
      foreach (entries, Record r)
      {
	 array path=r->fullpath/"/";
	 if (path[..sizeof(path)-2]==last) continue; // same dir
	 last=path[..sizeof(path)-2];

	 for (int i=0; i<sizeof(last); i++)
	    if (!filename_to_entry[last[..i]*"/"])
	       mkdirnode(last[..i]*"/",r,parent);
      }

      filenames=indices(filename_to_entry);
   }
};

class _TarFS
{
   inherit Filesystem.System;

   _Tar tar;

   static Stdio.File fd;    // tar file object
   static string filename;  // tar filename in parent filesystem

   void create(_Tar _tar,
	       string _wd,string _root,
	       Filesystem.Base _parent)
   {
      tar=_tar;
      wd=_wd;

      while (wd!="" && (wd[-1]=='/' || wd[-1]=='\\'))
	 wd=wd[..strlen(wd)-2];
      if (wd=="") wd="/";

      root=sscanf("%*[/]%s",_root)||_root;

      parent=_parent;
   }

   Filesystem.Stat stat(string file)
   {
      file=combine_path(wd,file);
      return tar->filename_to_entry[root+file];
   }

   array(string) get_dir(void|string directory,void|string|array globz)
   {
      directory=directory?combine_path(wd,directory):wd;

      array f=glob(root+directory+"?*",tar->filenames);
      f-=glob(root+directory+"*/*",f); // stay here

      return f;
   }

   Filesystem.Base cd(string directory)
   {
      Filesystem.Stat st=stat(directory);
      if (!st) return 0;
      if (st->isdir()) // stay in this filesystem
      {
	 object new=_TarFS(tar,st->fullpath,root,parent);
	 return new;
      }
      return st->cd(); // try something else
   }

   Stdio.File open(string filename,string mode)
   {

   }

   int access(string filename,string mode)
   {
      return 1; // sure
   }

   int rm(string filename)
   {
   }

   void chmod(string filename,int|string mode)
   {
   }

   void chown(string filename,int|object owner,int|object group)
   {
   }
}

class `()
{
   inherit _TarFS;

   void create(string filename,void|object(Filesystem.Base) parent)
   {
      if (!parent) parent=Filesystem.System();

      object fd=parent->open(filename,"r");
      if (!fd) 
	 error("Not a Tar file\n");

      object tar=_Tar(fd,filename,this_object());

      _TarFS::create(tar,"/","",parent);
   }
}


