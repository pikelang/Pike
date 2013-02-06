#pike __REAL_VERSION__

//! @decl void create(string filename, void|Filesystem.Base parent,@
//!                   void|object file)
//! Filesystem which can be used to mount a ZIP file.
//!
//! @param filename
//! The tar file to mount.
//! @param parent
//! The parent filesystem. If non is given, the normal system
//! filesystem is assumed. This allows mounting a ZIP-file within
//! a zipfile.
//! @param file
//! If specified, this should be an open file descriptor. This object
//! could e.g. be a @[Stdio.File] or similar object.

//! A class for reading and writing ZIP files.
//! 
//! Note that this class does not support the full ZIP format 
//! specification, but rather only the most common features.
//!
//! Storing, Deflating and Bzip2 are supported storage methods.
//!
//! Notably, large files, encryption and passwords are not 
//! supported.

#if constant(Gz.deflate)

Stdio.File fd;

// Structures.
constant L_COMP_STORE  = 0;
constant L_COMP_DEFLATE = 8;
constant L_COMP_BZIP2 = 12;

typedef int short;
typedef int long;

class _Zip
{
  object fd;
  string filename;

  array entries = ({});

  int compression_value = 6;

//! sets the compression value (0 to 9)
void set_compression_value(int(0..9) v)
{
  compression_value = v;
}

class CentralRecord
{
  inherit Filesystem.Stat;
        
  constant this_size = 4*6 + 11*2;
  long signature;

  short version_made_by;
  short ver_2_extract;
  short general_flags;
  short comp_method;
  short date;
  short time;

  long crc32;
  long comp_size;
  long uncomp_size;

  short filename_len;
  short extra_len;
  short comment_len;
  short start_disk;
  short int_file_attr;

  long ext_file_attr;
  long local_offset;

  string filename;
  string extra;
  string comment;
  
  void create()
  {
      sscanf( fd->read( this_size ),
            "%-4c" + "%-2c"*6 + "%-4c"*3 +  "%-2c"*5 + "%-4c"*2,
            signature, version_made_by, ver_2_extract, general_flags,
            comp_method, date, time, crc32, comp_size, uncomp_size,
            filename_len, extra_len, comment_len, start_disk, 
            int_file_attr, ext_file_attr, local_offset );

      filename = fd->read( filename_len );

      extra = fd->read( extra_len );      
      parse_extra(extra);

      object fds;
      if(fd) 
        fds = fd->stat();
            
      if(!uid && fds)
        uid = fds->uid;

      if(!gid && fds)
        gid = fds->gid;
        
      if(filename[-1] == '/')
      {   
      // if the extra field doesn't specify the mode, we fake it by using the parent file's.
      // note that we need to figure out a special dispensation for directories in this case.

        if(!mode && fds)
        {
          mode = fds->mode;
          mode |= (!!(mode&256) << 6);
          mode |= (!!(mode&32) << 3);
          mode |= (!!(mode&4) << 0);
        }
        set_type("dir");
        filename = filename[..<1];
      }
      else
      {
        if(!mode && fds)
          mode = fds->mode;

        set_type("reg");
      }
      name = (filename/"/")[-1];
      fullpath = filename;
      
      comment = fd->read( comment_len );
      filename = replace( filename, "\\", "/" );
      if( filename_len )
        while( filename[0] == '/' )
          filename = filename[1..];
      
      mtime = date_dos2unix(date, time);
      atime = ctime = mtime;
      size = uncomp_size;
      
      central_records[(filename)] = this;
  }

  LocalFileRecord local_record;
  LocalFileRecord open()
  {
    if( !local_record )
      local_record = LocalFileRecord( local_offset, this );
    return local_record;
  }

void parse_extra(string extra)
{
   int id, len;
   string val;
   while(sizeof(extra))
   {
     sscanf(extra, "%-2c%-2c%s", id, len, extra);
     if(len)
       sscanf(extra, "%" + len + "s%s", val, extra);
     string rest;
     int ver;
     
     switch(id)
     {
        
        case 0x000d: // UNIX extension
          // unix
          sscanf(val, "%-4c%-4c%-2c%-2c", atime, mtime, uid, gid);
          break;     

          case 0x5455: // Extended Timestamp extension
            int info;
            sscanf(val, "%-1c%-4c", info, mtime);
            break;
          
          case 0x7075: // UTF8 Name extension
            int crc;
            sscanf(rest, "%-1c%-4c%s", ver, crc, rest);
            filename = utf8_to_string(rest);

          case 0x7875: // New UNIX extension
            // unix
            int l;
            sscanf(val, "%-1c%-1c%s", ver, l, rest);
            if(ver != 1) break;
            sscanf(rest, "%-" +l + "c%-1c%s", uid, l, rest);
            sscanf(rest, "%-" +l + "c%-1c%s", gid, l, rest);
            break;     

     }
   }
}

}
/*

        local file header signature     4 bytes  (0x04034b50)
        version needed to extract       2 bytes
        general purpose bit flag        2 bytes
        compression method              2 bytes
        last mod file time              2 bytes
        last mod file date              2 bytes
        crc-32                          4 bytes
        compressed size                 4 bytes
        uncompressed size               4 bytes
        file name length                2 bytes
        extra field length              2 bytes

        file name (variable size)
        extra field (variable size)

*/
class LocalFileRecord
{
  inherit Filesystem.Stat;
  
  constant this_size = 4*4 + 7*2;

  long signature;       //< local file header signature 4 bytes (0x04034b50)

  short ver_2_extract;  //< version needed to extract
  short general_flags;  //< general purpose bit flag
  short comp_method;    //< compression method
  short date;           //< last mod file time
  short time;           //< last mod file date

  long crc32;           //< crc-32
  long comp_size;       //< compressed size
  long uncomp_size;     //< uncompressed size

  short filename_len;   //< file name length
  short extra_len;      //< extra field length

  string filename;
  string extra;
  string comment;

  long data_offset;
  string data;
  object _fd;

  string _sprintf(mixed t)
  {
    return "LocalFileRecord(" + filename + ")";
  }  

  void create(int | mapping entry, object|void central_record)
  {
    decode(entry, central_record);
  }

  int get_data_length()
  {
    return data_offset + comp_size;
  }


  void decode( int offset, object|void central_record )
  {
    fd->seek( offset );
    sscanf( fd->read( this_size ),
            "%-4c" + "%-2c"*5 + "%-4c"*3 + "%-2c"*2,
            signature, ver_2_extract, general_flags,
            comp_method, date, time, crc32, comp_size,
            uncomp_size, filename_len, extra_len );

    filename = fd->read( filename_len );
    if(filename[-1] == '/')
      set_type("dir");
    extra = fd->read( extra_len );

    if( offset+filename_len+extra_len+this_size != fd->tell() )
      error("Truncated ZIP\n");

    data_offset = fd->tell();

    if(general_flags & 0x08) 
    {
      // lengths were not available at header creation. use central record.
      crc32 = central_record->crc32;
      uncomp_size = central_record->uncomp_size;
      comp_size = central_record->comp_size;
    }
  }

  string read()
  {
    int check;
    string data;

    data = low_read();
    check = (Gz.crc32(data) & 0xffffffff);
  
    if(check != crc32)
    {
      throw(Error.Generic(sprintf("Bad CRC for file %O: expected %x, got %x.\n", filename, crc32, check)));
    }

    return data;
  }

  string low_read()
  {
    int check;
    fd->seek( data_offset );
    string data = fd->read( comp_size );

    switch( comp_method )
    {
      case L_COMP_STORE:
        return data;
#if constant(Bz2.Inflate)
      case L_COMP_BZIP2:
        return Bz2.Inflate()->inflate(data);
#endif
      case L_COMP_DEFLATE:
        return Gz.inflate()->inflate( sprintf("%1c%1c", 8, ((310-8)<<8)%31) +
                                      data );
      default:
        error(sprintf("Unsupported compression method: %d\n", comp_method));
    }
  }
}

/*
 End of central directory record

        end of central dir signature    4 bytes  (0x06054b50)
        number of this disk             2 bytes
        number of the disk with the
        start of the central directory  2 bytes
        total number of entries in the
        central directory on this disk  2 bytes
        total number of entries in
        the central directory           2 bytes
        size of the central directory   4 bytes
        offset of start of central
        directory with respect to
        the starting disk number        4 bytes
        .ZIP file comment length        2 bytes
        .ZIP file comment       (variable size)

*/
class EndRecord
{
  long signature;
  short this_disk;
  short central_start_disk;
  short entries_here;
  short file_count;
  long central_size;
  long central_start_offset;
  short comment_len;
  
  constant this_size = (6 * 2) + (3 * 4);

  void create( )
  {
    int i;
    for( i = -10; i>-60000; i-- )
    {
      fd->seek( i );
      string data = fd->read( 4 );
      sscanf( data, "%-4c", signature );
      
      if( signature == 0x06054b50 )
        break;
    }
    if( i < -50000 )
      error("Could not find Zip-file index\n");

    fd->seek( i );
    sscanf( fd->read(this_size), ("%-4c" + "%-2c"*4 + "%-4c"*2 + "%-2c"),
            signature, this_disk, central_start_disk, entries_here,
            file_count, central_size, central_start_offset,
            comment_len );

    if( (this_disk != central_start_disk) )
      error("Could not find Zip-file index\n");
    
    fd->seek( central_start_offset );
    for( i = 0; i<file_count; i++ )
      CentralRecord( );
  }
}


// EndRecord end;

mapping central_records = ([]);

string read_flat( string file )
{
  return read( "\0"+file );
}

string read( string file )
{
  file = lower_case( file );
  while( strlen(file) && file[0] == '/' )
    file = file[1..];
  if( central_records[file] )
    return central_records[file]->open()->read();
}

array(string) get_dir_flat()
{
  array res = ({});
  foreach( central_records; string d; object rec )
  {
    if( sizeof(d) && !d[0] )
      res += ({ d[1..] });
  }
  return res;
}

array(string) get_dir( string base )
{
  base = lower_case( base );
  while( sizeof(base) && base[0] == '/' )
    base = base[1..];
  if( sizeof(base) && base[-1] != '/' )
    base += "/";

  mapping res = ([]);
  foreach( central_records; string d; object rec )
  {
    if( strlen(d) && d[0] && sscanf( d, base+"%[^/]/", d ) )
      res[d] = 1;
  }
  return indices(res);
}

//!
void create(object fd, string filename, object parent)
{
  Gmp.bignum;
  this_program::filename = filename;
  this_program::fd = fd;
  int pos = 0; // fd is at position 0 here
   EndRecord(); 
}

void unzip(string todir)
{
  string start = "";

  low_unzip(start, todir);
}

void low_unzip(string start, string todir)
{
  foreach(get_dir(start);; string fn)
  {
    string rfn = combine_path(start, fn);
    string s;
    if(sizeof(rfn) && (rfn[-1] == '/')) continue;
    else if(s = read(rfn))
    {
      Stdio.write_file(combine_path(todir, rfn), s);
    }
    else
    {
      if(rfn[-1] == '/') rfn = rfn[0..sizeof(rfn)-2];
      mkdir(combine_path(todir, rfn));
      low_unzip(rfn, todir);
    }
  } 
}


//!
string generate()
{
  int offset = 0;
  int cdstart;
  int cdlength;
  String.Buffer buf = String.Buffer();

  foreach(entries;; object entry)
  {
    buf += entry->encode();
  }

  cdstart = sizeof(buf);

  foreach(entries;; object entry)
  {
    buf += entry->encode_central_record(offset);
    offset += entry->get_data_length();
  }

  cdlength = sizeof(buf) - cdstart;
  buf += encode_end_record(cdlength, cdstart);

  return buf->get();
}

string encode_end_record(int central_size, int central_start_offset, string|void comment)
{
  return sprintf (("%-4c" + "%-2c"*4 + "%-4c"*2 + "%-2c"),
            0x06054b50, 0, 0, sizeof(entries),
            sizeof(entries), central_size, central_start_offset,
            (comment?sizeof(comment):0) ) + (comment?comment:"");
}

//! adds a directory to an archive
void add_dir(string path, int recurse, string|void archiveroot)
{
  object i=Filesystem.System(path);

  if(!archiveroot) archiveroot = "";

  low_add_dir(i, archiveroot, recurse);

}

void low_add_dir(object i, string current_dir, int recurse)
{
  foreach(i->get_dir();; string fn)
  {
    if(fn == "CVS") continue; // never add CVS data
    if(i->stat(fn)->isdir())
    {
      if(recurse) low_add_dir(i->cd(fn), current_dir + "/" + fn, recurse);
    }
    else
    {
      add_file(current_dir + "/" + fn, i->open(fn, "r"));
    }
  }
}

//!
void add_file(string filename, string|Stdio.File data, int|void stamp, int|void no_compress)
{
  mapping entry = ([]);

  entry->filename = filename;
  entry->data = data;
  entry->stamp = stamp;
  entry->no_compress = no_compress;

  entries += ({LocalFileRecord(entry)});
}

// http://www.koders.com/cpp/fid254A78DD4D45BF598B2ACD5FA6C522D08D34F87F.aspx
/* Convert a MS-DOS time/date pair to a UNIX date (seconds since 1 1 70). 
*/

/* Linear day numbers of the respective 1sts in non-leap years. */
array day_n = ({ 0,31,59,90,120,151,181,212,243,273,304,334,0,0,0,0 });
		  /* JanFebMarApr May Jun Jul Aug Sep Oct Nov Dec */


int date_dos2unix(int time,int date)
{
	int month,year,secs;

	month = ((date >> 5) & 15)-1;
	year = date >> 9;
	secs = (time & 31)*2+60*((time >> 5) & 63)+(time >> 11)*3600+86400*
	    ((date & 31)-1+day_n[month]+(year/4)+year*365-((year & 3) == 0 &&
	    month < 2 ? 1 : 0)+3653);
			/* days since 1.1.70 plus 80's leap day */
	return secs;
}


/* Convert linear UNIX date to a MS-DOS time/date pair. */
//! @returns 
//!  an array containing ({time, date})
array date_unix2dos(int unix_date)
{
        int date, time;
	int day,year,nl_day,month;

	time = (unix_date % 60)/2+(((unix_date/60) % 60) << 5)+
	    (((unix_date/3600) % 24) << 11);
	day = unix_date/86400-3652;
	year = day/365;
	if ((year+3)/4+365*year > day) year--;
	day -= (year+3)/4+365*year;
	if (day == 59 && !(year & 3)) {
		nl_day = day;
		month = 2;
	}
	else {
		nl_day = (year & 3) || day <= 59 ? day : day-1;
		for (month = 0; month < 12; month++)
			if (day_n[month] > nl_day) break;
	}
	date = nl_day-day_n[month-1]+1+(month << 5)+(year << 9);

  return ({time,date});

}

}

class _ZipFS
{
  inherit Filesystem.System;

  _Zip zip;

  void create(_Zip _zip,
	      string _wd, string _root,
	      Filesystem.Base _parent)
  {
    zip = _zip;

    sscanf(reverse(_wd), "%*[\\/]%s", wd);
    wd = reverse(wd);
    if(wd=="")
      wd = "/";

    sscanf(_root, "%*[/]%s", root);
    parent = _parent;
  }

  string _sprintf(int t)
  {
    return  t=='O' && sprintf("_ZipFS(/* root=%O, wd=%O */)", root, wd);
  }

  Filesystem.Stat stat(string file, void|int lstat)
  {
    string d;
    d = combine_path_unix(wd, file);
    if(d[0] == '/') d = d[1..];
    return zip->central_records[d];
  }

  array(string) get_dir(void|string directory, void|string|array globs)
  {
    string d;
    directory = combine_path_unix(wd, (directory||""), "");
    d = root+directory;
    if(d[0] == '/') d = d[1..];
    array f = glob(d+"?*", indices(zip->central_records));
    f -= glob(d+"*/?*", f); // stay here

    foreach(f;int x; string fn)
    {
       if(fn[-1] == '/')
         f[x] = (fn/"/")[-2];
       else
         f[x] = (fn/"/")[-1];
    }
    return f;
  }

  Filesystem.Base cd(string directory)
  {
    Filesystem.Stat st = stat(directory);
    if(!st) return 0;
    if(st->isdir) // stay in this filesystem
    {
      object new = _ZipFS(zip, st->fullpath, root, parent);
      return new;
    }
    return st->cd(); // try something else
  }

  Stdio.File open(string filename, string mode)
  {
    string d;
          
    d = combine_path_unix(wd, filename);
    if(d[0] == '/') d = d[1..];
    
    return zip->central_records[d] &&
	   Stdio.FakeFile(zip->central_records[d]->open(mode)->read());
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
  inherit _ZipFS;

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
      error("Not a Zip file\n");

    _Zip zip = _Zip(fd, filename, this);

    _ZipFS::create(zip, "/", "", parent);
  }

  string _sprintf(int t)
  {
    return t=='O' &&
      sprintf("Filesystem.zip(/* zip->filename=%O, root=%O, wd=%O */)",
	      zip && zip->filename, root, wd);
  }
}

#endif /* constant(Gz.deflate) */
