/*
 * $Id: Tar.pmod,v 1.27 2004/02/27 16:20:43 bill Exp $
 */

#pike __REAL_VERSION__

//! @decl void create(string filename, void|Filesystem.Base parent,@
//!                   void|object file)
//! Filesystem which can be used to mount a Tar file.
//!
//! @param filename
//! The tar file to mount.
//! @param parent
//! The parent filesystem. If non is given, the normal system
//! filesystem is assumed. This allows mounting a TAR-file within
//! a tarfile.
//! @param file
//! If specified, this should be an open file descriptor. This object
//! could e.g. be a @[Stdio.File], @[Gz.File] or @[Bz2.File] object.

class _Tar  // filesystem
{
  object fd;
  string filename;

  class ReadFile
  {
    inherit Stdio.FakeFile;

    static private int start, pos, len;

    static string _sprintf(int t)
    {
      return t=='O' && sprintf("Filesystem.Tar.ReadFile(%d, %d /* pos = %d */)",
		     start, len, pos);
    }

    void create(int p, int l)
    {
//      assign(fd/*->dup()*/);
      start = p;
      len = l;
      fd->seek(start);
      ::create(fd->read(len));
      seek(0);
    }
  }

  class Record
  {
    inherit Filesystem.Stat;

    constant RECORDSIZE = 512;
    constant NAMSIZ = 100;
    constant TUNMLEN = 32;
    constant TGNMLEN = 32;
    constant SPARSE_EXT_HDR = 21;
    constant SPARSE_IN_HDR = 4;

    string arch_name;

    int linkflag;
    string arch_linkname;
    string magic;
    int devmajor;
    int devminor;
    int chksum;

    int pos;
    int pseudo;

    // Header description:
    //
    // Fieldno	Offset	len	Description
    // 
    // 0	0	100	Filename
    // 1	100	8	Mode (octal)
    // 2	108	8	uid (octal)
    // 3	116	8	gid (octal)
    // 4	124	12	size (octal)
    // 5	136	12	mtime (octal)
    // 6	148	8	chksum (octal)
    // 7	156	1	linkflag
    // 8	157	100	linkname
    // 9	257	8	magic
    // 10	265	32				(USTAR) uname
    // 11	297	32				(USTAR)	gname
    // 12	329	8	devmajor (octal)
    // 13	337	8	devminor (octal)
    // 14	345	167				(USTAR) Long path
    //
    // magic can be any of:
    //   "ustar\0""00"	POSIX ustar (Version 0?).
    //   "ustar  \0"	GNU tar (POSIX draft)

    void create(void|string s, void|int _pos)
    {
      if(!s)
      {
	pseudo = 1;
	return;
      }

      pos = _pos;
      array a = array_sscanf(s,
			     "%"+((string)NAMSIZ)+"s%8s%8s%8s%12s%12s%8s"
			     "%c%"+((string)NAMSIZ)+"s%8s"
			     "%"+((string)TUNMLEN)+"s"
			     "%"+((string)TGNMLEN)+"s%8s%8s%167s");
      sscanf(a[0], "%s%*[\0]", arch_name);
      sscanf(a[1], "%o", mode);
      sscanf(a[2], "%o", uid);
      sscanf(a[3], "%o", gid);
      sscanf(a[4], "%o", size);
      sscanf(a[5], "%o", mtime);
      sscanf(a[6], "%o", chksum);
      linkflag = a[7];
      sscanf(a[8], "%s%*[\0]", arch_linkname);
      sscanf(a[9], "%s%*[\0]", magic);

      if((magic=="ustar  ") || (magic == "ustar"))
      {
	// GNU ustar or POSIX ustar
	sscanf(a[10], "%s\0", uname);
	sscanf(a[11], "%s\0", gname);
	if (a[9] == "ustar\0""00") {
	  // POSIX ustar	(Version 0?)
	  string long_path = "";
	  sscanf(a[14], "%s\0", long_path);
	  if (sizeof(long_path)) {
	    arch_name = long_path + "/" + arch_name;
	  }
	} else if (arch_name == "././@LongLink") {
	  // GNU tar
	  // Data contains full filename of next record.
	  pseudo = 2;
	}
      }
      else
	uname = gname = 0;

      sscanf(a[12], "%o", devmajor);
      sscanf(a[13], "%o", devminor);

      fullpath = combine_path_unix("/", arch_name);
      name = (fullpath/"/")[-1];
      atime = ctime = mtime;

      set_type( ([  0:"reg",
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

    object open(string mode)
    {
      if(mode!="r")
	error("Can only read right now.\n");
      return ReadFile(pos, size);
    }
  };

  array(Record) entries = ({});
  array filenames;
  mapping filename_to_entry;

  void mkdirnode(string what, Record from, object parent)
  {
    Record r = Record();

    if(what=="") what = "/";

    r->fullpath = what;
    r->name = (what/"/")[-1];

    r->mode = 0755|((from->mode&020)?020:0)|((from->mode&02)?02:0);
    r->set_type("dir");
    r->uid = 0;
    r->gid = 0;
    r->size = 0;
    r->atime = r->ctime = r->mtime = from->mtime;
    r->filesystem = parent;

    filename_to_entry[what] = r;
  }

  void create(object fd, string filename, object parent)
  {
    this_program::filename = filename;
    // read all entries

    this_program::fd = fd;
    int pos = 0; // fd is at position 0 here
    string next_name;
    for(;;)
    {
      Record r;
      string s = this_program::fd->read(512);

      if(s=="" || sizeof(s)<512 || sscanf(s, "%*[\0]%*2s")==1)
	break;

      r = Record(s, pos+512);
      r->filesystem = parent;

      if(r->arch_name!="" && !r->pseudo) {  // valid file?
	if(next_name) {
	  r->fullpath = next_name;
	  r->name = (next_name/"/")[-1];
	  next_name = 0;
	}
	entries += ({ r });
      }
      if(r->pseudo==2)
	next_name = combine_path("/", r->open("r")->read(r->size-1));

      pos += 512 + r->size;
      if(pos%512)
	pos += 512 - (pos%512);
      this_program::fd->seek(pos);
    }

    filename_to_entry = mkmapping(entries->fullpath, entries);

    // create missing dirnodes

    array last = ({});
    foreach(entries, Record r)
    {
      array path = r->fullpath/"/";
      if(path[..sizeof(path)-2]==last) continue; // same dir
      last = path[..sizeof(path)-2];

      for(int i = 0; i<sizeof(last); i++)
	if(!filename_to_entry[last[..i]*"/"])
	  mkdirnode(last[..i]*"/", r, parent);
    }

    filenames = indices(filename_to_entry);
  }

  string _sprintf(int t)
  {
    return t=='O' && sprintf("_Tar(/* filename=%O */)", filename);
  }
}

class _TarFS
{
  inherit Filesystem.System;

  _Tar tar;

  void create(_Tar _tar,
	      string _wd, string _root,
	      Filesystem.Base _parent)
  {
    tar = _tar;

    sscanf(reverse(_wd), "%*[\\/]%s", wd);
    wd = reverse(wd);
    if(wd=="")
      wd = "/";

    sscanf(_root, "%*[/]%s", root);
    parent = _parent;
  }

  void destroy() {
    if(tar && tar->fd) tar->fd->close();
  }

  string _sprintf(int t)
  {
    return  t=='O' && sprintf("_TarFS(/* root=%O, wd=%O */)", root, wd);
  }

  Filesystem.Stat stat(string file, void|int lstat)
  {
    file = combine_path_unix(wd, file);
    return tar->filename_to_entry[root+file];
  }

  array(string) get_dir(void|string directory, void|string|array globs)
  {
    directory = combine_path_unix(wd, (directory||""), "");

    array f = glob(root+directory+"?*", tar->filenames);
    f -= glob(root+directory+"*/*", f); // stay here

    return f;
  }

  Filesystem.Base cd(string directory)
  {
    Filesystem.Stat st = stat(directory);
    if(!st) return 0;
    if(st->isdir()) // stay in this filesystem
    {
      object new = _TarFS(tar, st->fullpath, root, parent);
      return new;
    }
    return st->cd(); // try something else
  }

  Stdio.File open(string filename, string mode)
  {
    filename = combine_path_unix(wd, filename);
    return tar->filename_to_entry[root+filename] &&
	   tar->filename_to_entry[root+filename]->open(mode);
  }

  int access(string filename, string mode)
  {
    return 1; // sure
  }

  int rm(string filename)
  {
  }

  void chmod(string filename, int|string mode)
  {
  }

  void chown(string filename, int|object owner, int|object group)
  {
  }
}

class `()
{
  inherit _TarFS;

  void create(string|object filename, void|Filesystem.Base parent,
	      void|object f)
  {
    if(!parent) parent = Filesystem.System();

    object fd;

    if(f)
      fd = f;
    else 
      fd = parent->open(filename, "r");

    if(!fd)
      error("Not a Tar file\n");

    _Tar tar = _Tar(fd, filename, this);

    _TarFS::create(tar, "/", "", parent);
  }

  string _sprintf(int t)
  {
    return t=='O' &&
      sprintf("Filesystem.Tar(/* tar->filename=%O, root=%O, wd=%O */)",
	      tar && tar->filename, root, wd);
  }
}

