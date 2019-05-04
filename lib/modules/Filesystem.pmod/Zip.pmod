#pike __REAL_VERSION__
#require constant(Gz.deflate)

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

Stdio.File fd;

// Compression mechanisms
constant L_COMP_STORE  = 0;
constant L_COMP_DEFLATE = 8;
constant L_COMP_BZIP2 = 12;

typedef int short;
typedef int long;

//! traditional Zip encryption is CRC32 based, and rather insecure.
//! support here exists to ease transition and to work with legacy files.
class Decrypt
{
  private array key = ({305419896, 591751049, 878082192});
  private array tab = allocate(256);

  protected void create(string pw)
  {
    gentab();
    foreach(pw;; int ch)
    {
      update_keys(ch);
    }
  }

  // populate the crc table
  private void gentab()
  {
    int poly = 0xedb88320;
    for(int i; i<sizeof(tab); i++)
    {
      int crc = i;
      for(int j = 0; j < 8; j++)
      {
        if(crc & 1)
          crc = ((crc >> 1) & 0x7fffffff) ^ poly;
        else
          crc = ((crc >> 1) & 0x7fffffff);
      }
      tab[i] = crc;
    }
  }

  // single byte crc
  private int crc32(int c, int crc)
  {
    return ((crc >> 8) & 0xffffff) ^ tab[(crc ^ c) & 0xff];
  }

  private void update_keys(int ch)
  {
    key[0] = crc32(ch, key[0]);
    key[1] = (key[1] + (key[0] & 255)) & 0xffffffff;
    key[1] = ((key[1] * 134775813) + 1) & 0xffffffff;
    key[2] = crc32((key[1] >> 24) & 255, key[2]);
  }

  private int decrypt_char(int c)
  {
    int k = key[2] | 2;
    c = c ^ (((k * (k^1)) >> 8) & 255);
    update_keys(c);

    return c;
  }

  //! decrypt a string
  //!
  //! @param x
  //!   encrypted string to decrypt
  string decrypt(string x)
  {
    String.Buffer buf = String.Buffer();

    foreach(x;; int c)
      buf->putchar(decrypt_char(c));

    return buf->get();
  }
}

//! A class for reading and writing ZIP files.
//!
//! Note that this class does not support the full ZIP format
//! specification, but does support the most common features.
//!
//! Storing, Deflating and Bzip2 (if the Bz2 module is available) are
//! supported storage methods.
//!
//! This class is able to extract data from traditional ZIP password
//! encrypted archives.
//!
//! Notably, advanced PKware encryption (beyond reading traditional
//! password protected archives) and multi-part archives are not
//! currently supported.
class _Zip
{
  object fd;
  string filename;
  int use_zip64;
  int use_bzip2;

  protected string password;

  array entries = ({});

  int compression_value = 6;

  //! sets the compression value (0 to 9)
  void set_compression_value(int(0..9) v)
  {
    compression_value = v;
  }

  //! sets the password to be used for files encrypted using traditional
  //! PKZip encryption.
  void set_password(string pw)
  {
    password = pw;
  }

  //! enable zip64 extensions (large files) for this archive
  //!
  //! @note
  //! This setting may be used to force zip64 for files that do not
  //! otherwise require its use. If a file whose properties requires
  //! the use of zip65 is added to an archive, zip64 extensions will
  //! be enabled automatically.
  void set_zip64(int flag)
  {
    use_zip64 = flag;
  }

  //! is this archive using zip64 extensions?
  int is_zip64()
  {
    return use_zip64;
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

    protected void create()
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
        // if the extra field doesn't specify the mode, we fake it by
        // using the parent file's. note that we need to figure out a
        // special dispensation for directories in this case.

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
        case 0x0001: // ZIP64
          sscanf(val, "%-8c%-8c%-8c%-4c", uncomp_size, comp_size, local_offset, start_disk);
          break;

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
          sscanf(val, "%-1c%-4c%s", ver, crc, rest);
          filename = utf8_to_string(rest);

        case 0x7875: // New UNIX extension
          // unix
          int l;
          sscanf(val, "%-1c%-1c%s", ver, l, val);
          if(ver != 1) break;
          sscanf(val, "%-" +l + "c%-1c%s", uid, l, val);
          sscanf(val, "%-" +l + "c%-1c%s", gid, l, val);
          break;
        }
      }
    }
  } // end central record
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
    object _fd;

    protected string _sprintf(mixed t)
    {
      return "LocalFileRecord(" + filename + ")";
    }

    protected void create(int | mapping entry, object|void central_record)
    {
      if(mappingp(entry))
        populate(entry);
      else
        decode(entry, central_record);
    }

    //
    // entry keys
    //  no_compress
    //  bzip
    //  data
    //  stat
    //  filename
    //  stamp
    void populate(mapping entry)
    {
      signature = 0x04034b50;
      general_flags = 0;
      comp_method = ((!entry->data||entry->no_compress)?L_COMP_STORE:(entry->bzip2?L_COMP_BZIP2:L_COMP_DEFLATE)); // deflate

      if(entry->bzip2) use_bzip2 = 1;

      if(!entry->stat && objectp(entry->data))
        entry->stat = entry->data->stat();

      if(entry->stat)
        attach_statobject(entry->stat);
      // it appears that files are stored in utc without tz info

      if(size > 0xffffffff)
        use_zip64 = 1;

      filename = entry->filename;
      if(!entry->data) ;
      else if(objectp(entry->data))
        _fd = entry->data;
      else
        _fd = Stdio.FakeFile(entry->data);

      if(entry->stamp) mtime = entry->stamp;

      [time, date] = date_unix2dos(mtime - Calendar.Second(mtime)->utc_offset());

      if(comp_method == L_COMP_BZIP2)
        ver_2_extract = 46;
      else if(use_zip64)
        ver_2_extract = 45;
      else
        ver_2_extract = 2;
    }

    string encode_central_record(int offset)
    {
      encode_extra(offset);

      return sprintf(
             "%-4c" + "%-c%-c"+ "%-2c"*5 + "%-4c"*3 +  "%-2c"*5 + "%-4c"*2,
             0x02014b50, 3 /* UNIX */, ver_2_extract, ver_2_extract, general_flags,
             comp_method, time, date, crc32,
             (use_zip64?0xffffffff:comp_size),
             (use_zip64?0xffffffff:uncomp_size),
             filename?sizeof(filename):0, extra?sizeof(extra):0,
             comment?sizeof(comment):0, 0/*start_disk*/,
             0/*int_file_attr*/,
             0/*ext_file_attr*/,
             (use_zip64?0xffffffff:offset)) + (filename?filename:"") +
             (extra?extra:"") + (comment?comment:"");
    }

    string encode()
    {
      string cdata;
      string ucdata;

      if(_fd)
      {
        ucdata = _fd->read(0x7fffffff);
        uncomp_size += sizeof(ucdata||"");
        crc32 = Gz.crc32(ucdata, crc32);
        if(ucdata)
          cdata = write(ucdata);
        comp_size = sizeof(cdata||"");
      }

      encode_extra(-1);

      string ret = sprintf(
             "%-4c" + "%-2c"*5 + "%-4c"*5 + "%-2c"*2,
             signature, ver_2_extract, general_flags,
             comp_method, time, date, crc32,
             comp_size, uncomp_size,  (use_zip64?0xffffffff:comp_size),
             (use_zip64?0xffffffff:uncomp_size),
             filename?sizeof(filename):0,
             extra?sizeof(extra):0 );

      if(filename)
        ret += ( filename );
      if(extra)
        ret += ( extra );

      data_offset = sizeof(ret);

      return ret + (cdata||"");
    }

    void encode_extra(int|void offset)
    {
      string field;
      extra  = "";
      //    0x7075 UTF-8 filename
      field = string_to_utf8(filename);
      field = sprintf("%-1c%-4c%s", 1, Gz.crc32(field), field);
      extra += sprintf("%-2c%-2c%s", 0x7075, sizeof(field), field);

      // 0x000d UNIX
      string unixdata = "";
      unixdata = sprintf(("%-4c" * 2) + ("%-2c" * 2),
                         atime, mtime, uid, gid);
      extra += sprintf("%-2c%-2c%s", 0x000d, sizeof(unixdata), unixdata);

      // 0x7875 New UNIX
      unixdata = sprintf("%-1c%-1c%-" + get_int_size(uid) + "c%-1c%-" + get_int_size(gid) + "c",
        1,
        get_int_size(uid), uid,
        get_int_size(gid), gid);
      extra += sprintf("%-2c%-2c%s", 0x7875, sizeof(unixdata), unixdata);

      // 0x5455 extended timestamp
      unixdata = sprintf("%-1c%-4c", 0, mtime);
      extra += sprintf("%-2c%-2c%s", 0x5455, sizeof(unixdata), unixdata);
      if(use_zip64 && offset != -1)
      {
        unixdata = sprintf("%-8c%-8c%-8c%-4c", uncomp_size, comp_size, offset, 0 /* disk start */);
        extra += sprintf("%-2c%-2c%s", 0x0001, sizeof(unixdata), unixdata);
      }
    }

    int get_int_size(int val)
    {
      if (val < 0xffff) {
        if (val < 0xff)
          return 1; // 8 bit
        else
          return 2;// 16 bit
      } else {
        if (val < 0xffffffff)
          return 4;// 32 bit
        else
          return 8;// 64 bit
      }
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

      parse_extra(extra);

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
      if(general_flags >> 11 & 0x01)
      {
        // utf-8 encoded filename.
        filename = utf8_to_string(filename);
      }
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

    string write(string data)
    {
      switch(comp_method)
      {
      case L_COMP_STORE:
        return data;
#if constant(Bz2.Inflate)
      case L_COMP_BZIP2:
        return Bz2.Deflate()->deflate(data);
#endif
      case L_COMP_DEFLATE:
        return Gz.deflate(0-compression_value)->deflate(data);
      default:
        throw(Error.Generic("Unknown compression type " + comp_method + ".\n"));
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

      if(general_flags & 0x1)
      {
        if(!password)
          throw(Error.Generic("File encrypted, but no password supplied.\n"));

        object d = Decrypt(password);
        string head;

        sscanf(data, "%12s%s", head, data);

        head = d->decrypt(head);

        if(general_flags & 0x08) // if streaming, use high byte of date
        {
          check = (date >> 8) & 0xff;
        }
        else
        {
          check = (crc32 >> 24) & 0xff;
        }

        if(check != head[-1])
        {
          throw(Error.Generic("Invalid password supplied.\n"));
        }

        data = d->decrypt(data);
      }

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

  class EndRecord64
  {
/*
 Zip64 end of central directory record

        zip64 end of central dir
        signature                       4 bytes  (0x06064b50)
        size of zip64 end of central
        directory record                8 bytes
        version made by                 2 bytes
        version needed to extract       2 bytes
        number of this disk             4 bytes
        number of the disk with the
        start of the central directory  4 bytes
        total number of entries in the
        central directory on this disk  8 bytes
        total number of entries in the
        central directory               8 bytes
        size of the central directory   8 bytes
        offset of start of central
        directory with respect to
        the starting disk number        8 bytes
        zip64 extensible data sector    (variable size)

*/

    constant this_size = 12; // the size of the header only.

    int signature;
    int version_made_by;
    int version_2_ext;
    int this_disk;
    int central_start_disk;
    int entries_here;
    int file_count;
    int central_size;
    int central_start_offset;
    string extra;

    protected void create(int offset)
    {
      int full_size;
      int i;
      fd->seek( offset );
      string data = fd->read( 4 );
      sscanf(data, "%-4c", signature );

      if( signature != 0x06064b50 )
        error("Could not find Zip64 EndDirectory record\n");

      fd->seek( offset );
      sscanf( fd->read(this_size), ("%-4c%-8c"), signature, full_size);

      sscanf(fd->read(full_size), "%-2c"*2 + "%-4c"*2 + ("%-8c"*4) + "%s",
             version_made_by, version_2_ext, this_disk, central_start_disk,
             entries_here, file_count, central_size, central_start_offset,
             extra );

      if( (this_disk != central_start_disk) )
        error("Could not find Zip-file index\n");

      fd->seek( central_start_offset );

      for( i = 0; i<file_count; i++ )
      {
        CentralRecord( );
      }
    }

  }
  // EndRecord64 end;


  class EndRecordLocator64
  {
/*
 Zip64 end of central directory locator

      zip64 end of central dir locator
      signature                       4 bytes  (0x07064b50)
      number of the disk with the
      start of the zip64 end of
      central directory               4 bytes
      relative offset of the zip64
      end of central directory record 8 bytes
      total number of disks           4 bytes
*/

    constant this_size = 20; // the size of the header only.

    int signature;
    int central_end_disk;
    int central_end_offset;
    int disk_count;

    protected void create(int from)
    {
      int i;

      for( i = from-this_size; i > from-50; i-- )
      {
        fd->seek( i );
        string data = fd->read( 4 );
        sscanf( data, "%-4c", signature );

        if( signature == 0x07064b50 )
        {
          use_zip64 = 1;
          break;
        }
      }

      if(!use_zip64)
        return;

      use_zip64 = 1;
      fd->seek( i );
      sscanf( fd->read(this_size), ("%-4c%-4c%-8c%-4c"),
              signature, central_end_disk,
              central_end_offset, disk_count);

      //   if( (this_disk != central_end_disk) )
      //     error("Could not find Zip-file index\n");

      EndRecord64(central_end_offset);
    }
  }

// EndRecordLocator64 end;

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

    protected void create()
    {
      int i;
      for( i = -10; i>-(0xffff+23); i-- )
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

      EndRecordLocator64(i);

      if(!use_zip64)
      {
        fd->seek( central_start_offset );
        for( i = 0; i<file_count; i++ )
          CentralRecord();
      }
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
  protected void create(object|void fd, string|void filename,
                        object|void parent)
  {
    this_program::filename = filename;
    this_program::fd = fd;

    if(!fd) return;

    // trigger parsing of the zip file
    EndRecord();
  }

  //!
  void unzip(string todir)
  {
    string start = "";

    low_unzip(start, todir);
  }

  protected void low_unzip(string start, string todir)
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


  //! generate the zip archive
  string generate()
  {
    int offset = 0;
    int cdstart;
    int cdlength;
    String.Buffer buf = String.Buffer();

    if(sizeof(entries) > 65535)
      use_zip64 = 1;

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

    if(use_zip64)
    {
      int record_start = sizeof(buf);
      buf += encode_end_record64(cdlength, cdstart);
      buf += encode_end_record_locator64(record_start);
    }
    cdlength = sizeof(buf) - cdstart;

    buf += encode_end_record(cdlength, cdstart);

    return buf->get();
  }

  protected string encode_end_record_locator64(int start)
  {
    return sprintf("%-4c%-4c%-8c%-4c", 0x07064b50,
                   0, start, 0);
  }

  protected string encode_end_record64(int cdlength, int cdstart)
  {
    string data = sprintf( "%-c%-c%-2c" + "%-4c"*2 + "%-8c"*4,
      45, 3, (use_bzip2?46:(use_zip64?45:2)), 0, 0, // NB this might be wrong
      sizeof(entries), sizeof(entries), cdlength, cdstart
    );
    return sprintf("%-4c%-8c%s", 0x06064b50, sizeof(data), data);
  }

  protected string encode_end_record(int central_size,
                                     int central_start_offset,
                                     string|void comment)
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
    object stat = file_stat(path);
    if(!archiveroot) archiveroot = "";

    low_add_dir(i, archiveroot, recurse, stat);
  }

  protected void low_add_dir(object i, string current_dir,
                             int recurse, object stat)
  {
    add_file(current_dir + "/", 0, stat);

    foreach(i->get_dir();; string fn)
    {
      if(i->stat(fn)->isdir)
      {
        if(recurse)
          low_add_dir(i->cd(fn), current_dir + "/" + fn, recurse, i->stat(fn));
      }
      else
      {
        add_file(current_dir + "/" + fn, i->open(fn, "r"));
      }
    }
  }

  //! add a file to an archive.
  //!
  void add_file(string filename, string|Stdio.File data,
                int|object|void stamp, int|void no_compress)
  {
    mapping entry = ([]);

    entry->filename = filename;
    entry->data = data;
    if(objectp(stamp))
      entry->stat = stamp;
    else
      entry->stamp = stamp;
    entry->no_compress = no_compress;

    entries += ({LocalFileRecord(entry)});
  }

  // http://www.koders.com/cpp/fid254A78DD4D45BF598B2ACD5FA6C522D08D34F87F.aspx
  // Convert a MS-DOS time/date pair to a UNIX date (seconds since 1 1 70).

  /* Linear day numbers of the respective 1sts in non-leap years. */
  constant  day_n = ({ 0,31,59,90,120,151,181,212,243,273,304,334,0,0,0,0 });
  /* JanFebMarApr May Jun Jul Aug Sep Oct Nov Dec */


  //! Convert MS-DOS time/date pair to a linear UNIX date.
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


  //! Convert linear UNIX date to a MS-DOS time/date pair.
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
// End class _Zip

class _ZipFS
{
  inherit Filesystem.System;

  _Zip zip;

  protected void create(_Zip _zip,
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

  //!
  void set_password(string pw)
  {
    zip->set_password(pw);
  }

  protected string _sprintf(int t)
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

  protected void create(string|object filename,
                        void|Filesystem.Base parent,
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

  protected string _sprintf(int t)
  {
    return t=='O' &&
      sprintf("Filesystem.zip(/* zip->filename=%O, root=%O, wd=%O */)",
	      zip && zip->filename, root, wd);
  }
}
