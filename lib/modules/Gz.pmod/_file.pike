#pike __REAL_VERSION__
#require constant(_Gz)

//! Low-level implementation of read/write support for GZip files

protected private Stdio.Stream f;
protected private .inflate inf;
protected private .deflate def;
protected private int level, strategy, window_size;
protected private string read_buf;
protected private int file_pos, crc, write_mode, at_eof;

final constant SEEK_SET = 0;
final constant SEEK_CUR = 1;
final constant SEEK_END = 2;

//! Opens a file for I/O.
//!
//! @param file
//!   The filename or an open filedescriptor or Stream for the GZip
//!   file to use.
//! @param mode
//!   Mode for the file operations. Defaults to read only. The
//!   following mode characters are unique to Gz.File.
//!
//!   @string
//!     @value "0"
//!     @value "1"
//!     @value "2"
//!     @value "3"
//!     @value "4"
//!     @value "5"
//!     @value "6"
//!     @value "7"
//!     @value "8"
//!     @value "9"
//!       Values 0 to 9 set the compression level from no
//!       compression to maximum available compression. Defaults to
//!       6.
//!     @value "f"
//!       Sets the compression strategy to @[FILTERED].
//!     @value "h"
//!       Sets the compression strategy to @[HUFFMAN_ONLY].
//!   @endstring
//!
//! @note
//!   If the object already has been opened, it will first be closed.
int open(string|int|Stdio.Stream file, void|string mode)
{
  close();
  write_mode = 0;
  level = 6;
  strategy = .DEFAULT_STRATEGY;
  window_size = 15;
  if(mode) {
    mode = filter(mode, lambda(int n) {
      		    if(n == 'w' || n == 'a')
      		      write_mode = 1;
      		    if(n >= '0' && n <= '9')
      		      level = n - '0';
      		    else if(n == 'f')
      		      strategy = .FILTERED;
      		    else if(n == 'h')
      		      strategy = .HUFFMAN_ONLY;
      		    else
      		      return 1;
      		  });
    if(write_mode)
      mode += "c"+(has_value(mode, 'a')? "w":"t");
  }
  at_eof = file_pos = 0;
  crc = .crc32("");
  if(objectp(file))
    f = file;
  else {
    f = Stdio.File();
    if(!f->open(file, mode||"rb"))
      return 0;
  }
  if (write_mode)
    return .make_header(f);
  if (mode = .check_header(f))
    sizeof(mode) && fill_read_buffer(mode);
  return !!mode;
}

//! Opens a gzip file for reading.
protected void create(void|string|Stdio.Stream gzFile, void|string mode)
{
  if(!zero_type(gzFile) && !open(gzFile, mode))
    error("Failed to open file.\n");
}

//! closes the file
//! @returns
//!  1 if successful
int close()
{
  if(def) {
    string s = def->deflate("", .FINISH);
    if(sizeof(s) && f->write(s) != sizeof(s))
      return 0;
    if(f->write(sprintf("%-4c%-4c", crc, file_pos)) != 8)
      return 0;
  }
  inf = 0;
  def = 0;
  read_buf = "";
  Stdio.File oldf = f;
  f = 0;
  return !oldf || oldf->close();
}

protected int fill_read_buffer(string|void data)
{
  if(at_eof)
    return 0;
  string r = data || f->read(16384);
  if(!sizeof(r)) {
    at_eof = 1;
    return 0;
  }
  if(!inf) inf = .inflate(-15);
  string b = inf->inflate(r);
  read_buf += b;
  crc = .crc32(b, crc);
  if(b = inf->end_of_stream()) {
    inf = 0;
    if(sizeof(b)<8)
      b += f->read(8-sizeof(b));
    sscanf(b, "%-4c%-4c", int f_crc, int f_len);
#ifdef GZ_FILE_DEBUG
    werror("File: crc=%x size=%d   Internal: crc=%x size=%d\n",
           f_crc, f_len, crc&0xffffffff, file_pos+sizeof(read_buf));
#endif
    if(f_crc != (crc&0xffffffff)) {
      // CRC error
      at_eof = 1;
      return 0;
    } else {
      crc = .crc32("");
      if(b = .check_header(f, b[8..]))
        sizeof(b) && fill_read_buffer(b);
      else
        at_eof = 1;
    }
  }
  return sizeof(r);
}

//! Reads len (uncompressed) bytes from the file.
//! If read is unsuccessful, 0 is returned.
int|string read(int len)
{
  while(sizeof(read_buf) < len)
    if(!fill_read_buffer())
      break;
  string res = read_buf[..len-1];
  read_buf = read_buf[len..];
  file_pos += sizeof(res);
  return res;
}

//! Writes the data to the file.
//! @returns
//!  the number of bytes written to the file.
int write(string data)
{
  if(!def) def = .deflate(-level, strategy, window_size);
  string comp = def->deflate(data, .NO_FLUSH);
  if(f->write(comp) != sizeof(comp))
    return 0;
  else {
    file_pos += sizeof(data);
    crc = .crc32(data, crc);
    return sizeof(data);
  }
}

//! Seeks within the file.
//! @param pos
//!   Position relative to the searchtype.
//! @param type
//!   SEEK_SET = set current position in file to pos
//!   SEEK_CUR = new position is current+pos
//!   SEEK_END is not supported.
//! @returns
//!   New position or negative number if seek failed.
int seek(int pos, void|int type)
{
  if(type != SEEK_SET && type != SEEK_CUR)
    return -1;
  if(write_mode) {
    if(type == SEEK_SET)
      pos -= file_pos;
    if(pos < 0)
      return -1;
    while(pos > 0) {
      int n = write("\0"*(pos>16384? 16384:pos));
      if(!n)
        return -1;
      pos -= n;
    }
    return file_pos;
  } else {
    if(type == SEEK_CUR)
      pos += file_pos;
    if(pos < 0)
      return -1;
    if(pos < file_pos) {
      if(!f->seek || f->seek(0)<0)
        return -1;
      at_eof = 0;
      file_pos = 0;
      inf = 0;
      read_buf = "";
      crc = .crc32("");
      string r = .check_header(f);
      if(r)
        sizeof(r) && fill_read_buffer(r);
      else
        return -1;
    } else
      pos -= file_pos;
    while(pos > 0) {
      string r = read(pos>16384? 16384:pos);
      if(!sizeof(r))
        return -1;
      pos -= sizeof(r);
    }
    return file_pos;
  }
}

//! @returns
//!  the current position within the file.
int tell()
{
  return file_pos;
}

//! @returns
//!  1 if EOF has been reached.
int(0..1) eof()
{
  if(at_eof) return 1;
  if(def || write_mode || sizeof(read_buf)) return 0;
  while(!sizeof(read_buf) && fill_read_buffer())
    ;
  return at_eof;
}

//! Sets the encoding level, strategy and window_size.
//!
//! @seealso
//!   @[Gz.deflate]
int setparams(void|int(0..9) level, void|int strategy,
 void|int(8..15) window_size) {
  if (def) {
    string s = def->deflate("", .SYNC_FLUSH);
    if (sizeof(s) && f->write(s) != sizeof(s))
      return 0;
    def = 0;
  }
  if (!zero_type(level))
    this_program::level = level;
  if (!zero_type(strategy))
    this_program::strategy = strategy;
  if (!zero_type(window_size))
    this_program::window_size = window_size;
  return 1;
}


