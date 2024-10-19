#pike __REAL_VERSION__

//! Filesystem which can be used to mount a Tar file.
//!
//! Two kinds of extended tar file records are supported:
//! @string
//!   @value "ustar\0\60\60"
//!	POSIX ustar (Version 0?).
//!   @value "ustar  \0"
//!     GNU tar (POSIX draft)
//! @endstring
//!
//! @note
//!   For a quick start, you probably want to use @[`()()].
//!
//! @seealso
//!   @[`()()]

constant EXTRACT_SKIP_MODE = 1;
//! Don't set any permission bits from the tar records.

constant EXTRACT_SKIP_EXT_MODE = 2;
//! Don't set set-user-ID, set-group-ID, or sticky bits from
//! the tar records.

constant EXTRACT_SKIP_MTIME = 4;
//! Don't set mtime from the tar records.

constant EXTRACT_CHOWN = 8;
//! Set owning user and group from the tar records.

constant EXTRACT_ERR_ON_UNKNOWN = 16;
//! Throw an error if an entry of an unsupported type is
//! encountered. This is ignored otherwise.

//! Low-level Tar Filesystem.
class _Tar
{
  object fd;
  string filename;

  protected int fd_in_use = -1;
  // If >= 0 then some ReadFile is in use. The value is the start
  // position of that file.

  class ReadFile
  {
    inherit Stdio.BlockFile;

    protected private int start, len;

    protected int cached_pos, cached_errno;
    // Used only when the fd has been released.

    protected string _sprintf(int t)
    {
      return t=='O' && sprintf("Filesystem.Tar.ReadFile(%d, %d)", start, len);
    }

    protected void allocate_fd()
    {
      if (fd_in_use >= 0 && fd_in_use != start)
	error ("Concurrent file use in %O.\n", _Tar::this);
      fd_in_use = start;
    }

    protected void release_fd()
    {
      if (fd_in_use == start) {
	if (fd->tell) cached_pos = fd->tell();
	if (fd->errno) cached_errno = fd->errno();
	fd_in_use = -1;
      }
    }

    protected void create(int p, int l)
    {
      start = p;
      len = l;
      allocate_fd();
      if (fd->seek(start) < 0)
	error ("Failed to seek to position %d in %O.\n", start, fd);
    }

    protected void _destruct()
    {
      fd_in_use = -1;
    }

    string read (void|int read_len)
    {
      if (start < 0) error ("File not open.\n");
      allocate_fd();
      int max_len = len - (fd->tell() - start);
      string res = fd->read (read_len ? min (read_len, max_len) : max_len);
      if (sizeof (res) == max_len)
	// For compatibility: If we read to the end then release the
	// fd for use from another ReadFile, so that it works if this
	// one isn't properly closed or destructed first.
	release_fd();
      return res;
    }

    void close()
    {
      release_fd();
      start = -1;
    }

    int seek (int to)
    {
      if (start < 0) error ("File not open.\n");
      allocate_fd();
      if (to < 0) to += len;
      if (to < 0 || to > len) return -1;
      return fd->seek (start + to);
    }

    int tell()
    {
      if (start < 0) error ("File not open.\n");
      if (fd_in_use != start)
	return cached_pos - start;
      else
	return fd->tell() - start;
    }

    int errno()
    {
      if (start < 0 || fd_in_use != start)
	return cached_errno;
      else
	return fd->errno();
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

    protected void create(void|string s, void|int _pos)
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

  protected void create(object fd, string filename, object parent)
  {
    this::filename = filename;
    // read all entries

    this::fd = fd;
    int pos = 0; // fd is at position 0 here
    string next_name;
    for(;;)
    {
      Record r;
      string s = this::fd->read(512);

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
      if (this::fd->seek(pos) < 0)
	error ("Failed to seek to position %d in %O.\n", pos, fd);
    }

    filename_to_entry = mkmapping(filenames = entries->fullpath, entries);

    // create missing dirnodes

    array last = ({});
    foreach(entries, Record r)
    {
      array path = r->fullpath/"/";
      if(path[..<1]==last) continue; // same dir
      last = path[..<1];

      for(int i = 0; i<sizeof(last); i++) {
	if(!filename_to_entry[last[..i]*"/"]) {
	  string dir = last[..i]*"/";
	  mkdirnode(dir, r, parent);
	  filenames += ({ dir });
	}
      }
    }

    // NB: It's legal to have several entries for the same path;
    //     the last one wins.
    filenames = Array.uniq(filenames);
  }

  protected void extract_bits (string dest, Stdio.Stat r, int which_bits)
  {
    if (!r->RECORDSIZE) {
      // This is just a Stdio.Stat and not a Record.
      //
      // Just restore the mode bits.
      which_bits = ~(EXTRACT_SKIP_MODE | EXTRACT_SKIP_EXT_MODE);
    }
#if constant (utime)
    if (!(which_bits & EXTRACT_SKIP_MTIME)) {
      mixed err = catch {
	  utime (dest, r->mtime, r->mtime, 1);
	};
      if (err) {
#ifdef __NT__
	// utime() is currently not supported for directories on WIN32.
	if (!r->isdir)
#endif
	  werror("Tar: Failed to set modification time for %O.\n", dest);
      }
    }
#endif

#if constant (chown)
    if (which_bits & EXTRACT_CHOWN) {
      int uid;
      if (array pwent = r->uname && getpwnam (r->uname))
	uid = pwent[2];
      else
	uid = r->uid;

      int gid;
      if (array grent = r->gname && getgrnam (r->gname))
	gid = grent[2];
      else
	gid = r->gid;

      chown (dest, uid, gid, 1);
    }
#endif

    if (!(which_bits & EXTRACT_SKIP_MODE) && !r->islnk) {
      if (which_bits & EXTRACT_SKIP_EXT_MODE)
	chmod (dest, r->mode & 0777);
      else
	chmod (dest, r->mode & 07777);
    }
  }

#if !constant(access)
  // Probably NT or similar single-user OS,
  // so pretend we can access anything.
  protected int access(string path, string|void flags) { return 1; }
#endif

  void extract (string src_dir, string dest_dir,
		void|string|function(string,Filesystem.Stat:int|string) filter,
		void|int flags)
  //! Extracts files from the tar file in sequential order.
  //!
  //! @param src_dir
  //!   The root directory in the tar file system to extract.
  //!
  //! @param dest_dir
  //!   The root directory in the real file system that will receive
  //!   the contents of @[src_dir]. It is assumed to exist and be
  //!   writable.
  //!
  //! @param filter
  //!   A filter for the entries under @[src_dir] to extract. If it's
  //!   a string then it's taken as a glob pattern which is matched
  //!   against the path below @[src_dir]. That path always begins
  //!   with a @expr{/@}. For directory entries it ends with a
  //!   @expr{/@} too, otherwise not.
  //!
  //!   If it's a function then it's called for every entry under
  //!   @[src_dir], and those where it returns nonzero are extracted.
  //!   The function receives the path part below @[src_dir] as the
  //!   first argument, which is the same as in the glob case above,
  //!   and the stat struct as the second. If the function returns a
  //!   string, it's taken as the path below @[dest_dir] where this
  //!   entry should be extracted (any missing directories are created
  //!   automatically).
  //!
  //!   If @[filter] is zero, then everything below @[src_dir] is
  //!   extracted.
  //!
  //! @param flags
  //!   Bitfield of flags to control the extraction:
  //!   @int
  //!     @value Filesystem.Tar.EXTRACT_SKIP_MODE
  //!       Don't set any permission bits from the tar records.
  //!     @value Filesystem.Tar.EXTRACT_SKIP_EXT_MODE
  //!       Don't set set-user-ID, set-group-ID, or sticky bits from
  //!       the tar records.
  //!     @value Filesystem.Tar.EXTRACT_SKIP_MTIME
  //!       Don't set mtime from the tar records.
  //!     @value Filesystem.Tar.EXTRACT_CHOWN
  //!       Set owning user and group from the tar records.
  //!     @value Filesystem.Tar.EXTRACT_ERR_ON_UNKNOWN
  //!       Throw an error if an entry of an unsupported type is
  //!       encountered. This is ignored otherwise.
  //!   @endint
  //!
  //! Files and directories are supported on all platforms, and
  //! symlinks are supported whereever @[symlink] exists. Other record
  //! types are currently not supported.
  //!
  //! @throws
  //! I/O errors are thrown.
  {
    if (!has_suffix (src_dir, "/")) src_dir += "/";
    if (has_suffix (dest_dir, "/")) dest_dir = dest_dir[..<1];

    int do_extract_bits =
      (flags & (EXTRACT_SKIP_MODE|EXTRACT_SKIP_MTIME|EXTRACT_CHOWN)) !=
      (EXTRACT_SKIP_MODE|EXTRACT_SKIP_MTIME);

    mapping(string:Stdio.Stat) restore_bits = ([]);

    // Similar to Stdio.mkdirhier(), but ensures that we have
    // read, write and execute permission on the resulting directory.
    //
    // The original directory permissions (if altered)
    // are registered in the variable "restore_bits" above.
    int mkdirhier(string pathname)
    {
      string path = "";
#ifdef __NT__
      pathname = replace(pathname, "\\", "/");
      if (pathname[1..2] == ":/" && `<=("A", upper_case(pathname[..0]), "Z"))
	path = pathname[..2], pathname = pathname[3..];
#endif
      if (pathname == "") pathname = ".";
      array(string) segments = pathname/"/";
      if (segments[0] == "") {
	path += "/";
	pathname = pathname[1..];
	segments = segments[1..];
      }
      // FIXME: An alternative could be a binary search,
      //        but since it is usually only the last few
      //        segments of the path that are missing, we
      //        just do a linear search from the end.
      int i = sizeof(segments);
      while (i--) {
	string p = path + segments[..i]*"/";
	Stdio.Stat st;
	if ((st = file_stat(p))) {
	  if (!st->isdir) return 0;
	  if (!access(p, "rwx")) {
	    // We're not allowed to write.
	    // Attempt to adjust the permissions temporarily.
	    catch {
	      chmod(p, st->mode | 0700);
	      if (!restore_bits[p]) {
		// Register p for permission restoration.
		restore_bits[p] = st;
	      }
	    };
	  }
	  break;
	}
      }
      if (!i && !sizeof(path) && !access(".", "rwx")) {
	// We're not allowed to write.
	// Attempt to adjust the permissions temporarily.
	catch {
	  Stdio.Stat st = file_stat(".");
	  chmod(".", st->mode | 0700);
	  if (!restore_bits["."]) {
	    // Register "." for permission restoration.
	    restore_bits["."] = st;
	  }
	};
      }
      i++;
      while (i < sizeof(segments)) {
	string p = path + segments[..i++] * "/";
	if (!mkdir(p, 0777)) {
	  if (errno() != System.EEXIST)
	    return 0;
	  else {
	    Stdio.Stat st;
	    if ((st = file_stat(p))) {
	      if (!st->isdir) return 0;
	      // Directory exists, but we couldn't access it
	      // in the earlier loop, so it was probably hidden.
	      if (!access(path, "rw")) {
		// We're not allowed to write.
		// Attempt to adjust the permissions temporarily.
		catch {
		  chmod(p, st->mode | 0700);
		  if (!restore_bits[p]) {
		    // Register p for permission restoration.
		    restore_bits[p] = st;
		  }
		};
	      }
	    }
	  }
	}
      }
      return Stdio.is_dir(path + pathname) && access(path + pathname, "rwx");
    };

    mixed err = catch {
	foreach (entries, Record r) {
	  string fullpath = r->fullpath;
	  if (has_prefix (fullpath, src_dir)) {
	    string subpath = fullpath[sizeof (src_dir) - 1..];
	    string|int filter_res;

	    if (!filter ||
		(stringp (filter) ? glob (filter, subpath) :
		 (filter_res = filter (subpath, r)))) {
	      string destpath = dest_dir +
		(stringp (filter_res) ? filter_res : subpath);

	      if (r->isdir) {
		if (!mkdirhier(destpath))
                  error ("Failed to create directory %q: %s.\n",
                         destpath, strerror(errno()));

		// Set bits etc afterwards on dirs.
		if (do_extract_bits)
		  restore_bits[destpath] = r;
	      }

	      else {
		string dest_dir = (destpath / "/")[..<1] * "/";
		if (!mkdirhier(dest_dir))
                  error ("Failed to create directory %q: %s.\n",
                         destpath, strerror(errno()));

		if (r->isreg) {
		  Stdio.File o = Stdio.File();
		  if (!o->open (destpath, "wct")) {
		    // Could be because the file exists, but
		    // with bad access permissions for us.
		    Stdio.Stat st = file_stat(destpath);
		    if (!st || catch {
			chmod(destpath, st->mode | 0600);
			if (!do_extract_bits)
			  restore_bits[destpath] = st;
		      } || !o->open (destpath, "wct")) {
                      error ("Failed to create %q: %s.\n",
			     destpath, strerror (o->errno()));
		    }
		  }

		  Stdio.BlockFile i = r->open ("r");
		  do {
		    string data = i->read (1024 * 1024);
		    if (data == "") break;
		    if (o->write (data) != sizeof (data))
                      error ("Failed to write %q: %s.\n",
			     destpath, strerror (o->errno()));
		  } while (1);

		  i->close();
		  o->close();

		  if (do_extract_bits)
		    extract_bits (destpath, r, flags);
		}

#if constant (symlink)
		else if (r->islnk) {
		  symlink (r->arch_linkname, destpath);

		  if (do_extract_bits)
		    extract_bits (destpath, r, flags);
		}
#endif

		else if (flags & EXTRACT_ERR_ON_UNKNOWN)
		  error ("Failed to extract entry of unsupported type %x: %O\n",
			 r->mode & 0xf000, r);
	      }
	    }
	  }
	}
      };

    // Make sure to restore directory permissions, etc
    // even if we've failed.
    if (sizeof(restore_bits)) {
      array(string) paths = indices(restore_bits);
      foreach (reverse(sort(paths)), string destpath) {
	mixed e = catch {
	    extract_bits(destpath, restore_bits[destpath], flags);
	  };
	if (!err) err = e;
      }
    }

    if (err) throw(err);
  }

  protected string _sprintf(int t)
  {
    return t=='O' && sprintf("_Tar(/* filename=%O */)", filename);
  }
}

//!
class _TarFS
{
  inherit Filesystem.System;

  _Tar tar;

  //!
  protected void create(_Tar _tar,
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

  protected string _sprintf(int t)
  {
    return  t=='O' && sprintf("_TarFS(/* root=%O, wd=%O */)", root, wd);
  }

  Filesystem.Stat stat(string file, void|int lstat)
  {
    file = combine_path_unix(wd, file);
    return tar->filename_to_entry[root+file];
  }

  protected string fixup_dir(string path, string dir)
  {
    path = path[sizeof(dir)..];
    return (path/"/")[0];
  }

  array(string) get_dir(void|string directory, void|string|array globs)
  {
    directory = combine_path_unix(wd, (directory||""), "");

    if (!has_suffix(directory, "/")) directory += "/";

    array(string) f =
      map(filter(tar->filenames, has_prefix, directory),
	  fixup_dir, directory);

    if (globs) {
      f = glob(globs, f);
    }

    return Array.uniq(f - ({ "" }));
  }

  object(Filesystem.Base)|zero cd(string directory)
  {
    Filesystem.Stat st = stat(directory);
    if(!st) return 0;
    if(st->isdir) // stay in this filesystem
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

  //! @fixme
  //!   Not implemented yet.
  int rm(string filename)
  {
  }

  //! @fixme
  //!   Not implemented yet.
  void chmod(string filename, int|string mode)
  {
  }

  //! @fixme
  //!   Not implemented yet.
  void chown(string filename, int|object owner, int|object group)
  {
  }
}

class `()
{
  inherit _TarFS;

  //! @decl void create(string filename, void|Filesystem.Base parent,@
  //!                   void|object file)
  //!
  //! @param filename
  //!   The tar file to mount.
  //!
  //! @param parent
  //!   The parent filesystem. If none is given, the normal system
  //!   filesystem is assumed. This allows mounting a TAR-file within
  //!   a tarfile.
  //!
  //! @param file
  //!   If specified, this should be an open file descriptor. This object
  //!   could e.g. be a @[Stdio.File], @[Gz.File] or @[Bz2.File] object.
  protected void create(string|object filename, void|Filesystem.Base parent,
			void|object file)
  {
    if(!parent) parent = Filesystem.System();

    if(!file)
      file = parent->open(filename, "r");

    if(!file)
      error("Not a Tar file\n");

    _Tar tar = _Tar(file, filename, this);

    _TarFS::create(tar, "/", "", parent);
  }

  protected string _sprintf(int t)
  {
    return t=='O' &&
      sprintf("Filesystem.Tar(/* tar->filename=%O, root=%O, wd=%O */)",
	      tar && tar->filename, root, wd);
  }
}
