//! Generate a .torrent binary string from files in the filesystem
//!
//! @example
//!   // usage: thisprogram [<file/dir>] [<file/dir>...] <target .torrent>
//!   int main(int ac,array am)
//!   {
//!      Generator g=Generator();
//!      foreach (am[1..sizeof(am)-2];;string f)
//!         g->add(f);
//!   
//!      string dest=am[-1];
//!      if (-1==search(dest,"torrent")) dest+=".torrent";
//!   
//!      Stdio.write_file(dest,g->digest());
//!      return 0;
//!   }

#pike __REAL_VERSION__

inherit .Torrent;

string base=0;
int offset=0;
int piece_size=262144;
string announce="http://";
array(array(string)) announce_list=({});

//! @decl void create(void|string base, void|int piece_size)
//! Create a generator. 
//!
//! @param base
//!   The base filename/path in the torrent.
//!
//! @param piece_size
//!   The size of the pieces that the SHA hashes are calculated on.
//!   Default 262144 and this value should probably be 2^n.
void create(void|string _base,
	    void|int _piece_size)
{
   targets=({});
   datamode="r";
   base=_base;
   if (_piece_size) piece_size=_piece_size;
}

//! Add one or multiple announcers (trackers). This is needed to get a
//! valid .torrent file. If this is called more then once, more
//! announcers (trackers) will be added with lower priority.
this_program add_announce(string|array(string) announce_url)
{
   if (stringp(announce_url))
   {
      if (!announce) announce=announce_url;
      announce_list+=({({announce_url})});
   }
   else
   {
      if (!announce) announce=announce_url[0];
      announce_list+=({announce_url});
   }
   return this;
}

//! Add a file to the torrent. The second argument
//! is what the file will be called in the torrent.
this_program add_file(string path,void|string filename)
{
   if (!filename) filename=(replace(path,"\\","/")/"/")[-1];
   if (!base) base=filename;

   Stdio.File fd=Stdio.File(path,"r");
   Stdio.Stat st=fd->stat();

   if (!st || !(<"reg","blk","chr">)[st->type])
      error("Not a regular file.\n");

   string f="";
   if (filename[..strlen(base)-1]==base)
      sscanf(filename[strlen(base)..],"%*[\\/]%s",f);
   else
      f=filename;

   Target t=Target(base,st->size,offset,f==""?0:f/"/");
   t->fd=fd;
   targets+=({t});
   
   offset+=st->size;
   
   return this;
}

//! Add a directory tree to the torrent. The second argument
//! is what the directory will be called in the torrent.
//! This will call add_file on all non-directories in the tree.
this_program add_directory_tree(string path,void|string dirbase)
{
   if (!dirbase) dirbase=(replace(path,"\\","/")/"/")[-1];
   if (!base) base=dirbase;

   array v=get_dir(path);
   if (!v) error("directory doesn't exist?\n");

   foreach (v-({".",".."});;string fn)
   {
      Stdio.Stat st=file_stat(combine_path(path,fn));
      if (st->isdir)
	 add_directory_tree(combine_path(path,fn),combine_path(dirbase,fn));
      else
	 add_file(combine_path(path,fn),combine_path(dirbase,fn));
   }

   return this;
}

//! Add a file, or a directory tree to the torrent.
//! This will call add_directory_tree or add_file.
this_program add(string path,void|string base)
{
   if (!file_stat(path))
      error("No such file or directory.\n");
   else if (file_stat(path)->isdir)
      return add_directory_tree(path,base);
   else
      return add_file(path,base);
}

//! Build the SHA hashes from the files.
void build_sha1s(void|function(int,int:void) progress_callback)
{
   info_sha1="";
   int n=(offset+piece_size-1)/piece_size;
   for (int i=0; i<offset; i+=piece_size)
   {
      if (progress_callback) progress_callback(i,offset);
      info_sha1+=Crypto.SHA->hash(targets->pread(i,piece_size)*"");
   }
   if (progress_callback && n<offset) progress_callback(offset,offset);
}

//! Finally make a torrent string out of this information.
//! Will call build_sha1's if the sha1's aren't generated already.
//!
//! @[progress_callback] is called with (pos,of) arguments, similar
//! to @[Torrent.verify_targets].
string digest(void|function(int,int:void) progress_callback)
{
   mapping info=([]);

   if (info_sha1=="" || !info_sha1)
      build_sha1s(progress_callback);
   
   info["creation date"]=time();
   if (announce) 
   {
      info["announce"]=announce;
      info["announce-list"]=announce_list;
   }

   info->info=([]);
   if (sizeof(targets)==1) info->info->length=offset;
   info->info->name=base;
   info->info["piece length"]=piece_size;
   info->info["pieces"]=info_sha1;
   
   if (sizeof(targets)>1)
   {
      info->info->files=({});
      foreach (targets;;Target t)
	 info->info->files+=
	    ({(["length":t->length,
		"path":t->path
	        // we might add md5 here too someday
	    ])});
   }

   return .Bencoding.encode(info);
}
