// $Id: module.pmod,v 1.1 2004/09/01 17:19:02 mast Exp $
#pike __REAL_VERSION__

inherit "../../../modules/Stdio.pmod/module.pmod";

//! @decl string read_file(string filename)
//! @decl string read_file(string filename, int start, int len)
//!
//! Read @[len] lines from a file @[filename] after skipping @[start] lines
//! and return those lines as a string. If both @[start] and @[len] are omitted
//! the whole file is read.
//!
//! @seealso
//! @[read_bytes()], @[write_file()]
//!
string read_file(string filename,void|int start,void|int len)
{
  FILE f;
  string ret, tmp;
  f=FILE();
  if(!f->open(filename,"r")) return 0;

  // Disallow devices and directories.
  Stat st;
  if (f->stat && (st = f->stat()) && !st->isreg) {
    error( "File %O is not a regular file!\n", filename );
  }

  switch(query_num_arg())
  {
  case 1:
    ret=f->read();
    break;

  case 2:
    len=0x7fffffff;
  case 3:
    while(start-- && f->gets());
    String.Buffer buf=String.Buffer();
    while(len-- && (tmp=f->gets()))
    {
      buf->add(tmp, "\n");
    }
    ret=buf->get();
    destruct(buf);
  }
  f->close();

  return ret;
}

//! @decl string read_bytes(string filename, int start, int len)
//! @decl string read_bytes(string filename, int start)
//! @decl string read_bytes(string filename)
//!
//! Read @[len] number of bytes from the file @[filename] starting at byte
//! @[start], and return it as a string.
//!
//! If @[len] is omitted, the rest of the file will be returned.
//!
//! If @[start] is also omitted, the entire file will be returned.
//!
//! @throws
//!   Throws an error if @[filename] isn't a regular file.
//!
//! @returns
//!   Returns @expr{0@} (zero) on failure to open @[filename].
//!
//!   Returns a string with the requested data otherwise.
//!
//! @seealso
//! @[read_file], @[write_file()], @[append_file()]
//!
string read_bytes(string filename, void|int start,void|int len)
{
  string ret;
  File f = File();

  if(!f->open(filename,"r"))
    return 0;
  
  // Disallow devices and directories.
  Stat st;
  if (f->stat && (st = f->stat()) && !st->isreg) {
    error( "File \"%s\" is not a regular file!\n", filename );
  }

  switch(query_num_arg())
  {
  case 1:
  case 2:
    len=0x7fffffff;
  case 3:
    if(start)
      if (f->seek(start) < 0) {f->close(); return 0;}
  }
  ret=f->read(len);
  f->close();
  return ret;
}

//! Write the string @[str] onto the file @[filename]. Any existing
//! data in the file is overwritten.
//!
//! For a description of @[access], see @[Stdio.File()->open()].
//!
//! @throws
//!   Throws an error if @[filename] couldn't be opened for writing.
//!
//! @returns
//!   Returns the number of bytes written.
//!
//! @seealso
//!   @[append_file()], @[read_bytes()], @[Stdio.File()->open()]
//!
int write_file(string filename, string str, int|void access)
{
  int ret;
  File f = File();

  if (query_num_arg() < 3) {
    access = 0666;
  }

  if(!f->open(filename, "twc", access))
    error("Couldn't open file "+filename+": " + strerror(f->errno()) + "\n");
  
  ret=f->write(str);
  f->close();
  return ret;
}

//! Append the string @[str] onto the file @[filename].
//!
//! For a description of @[access], see @[Stdio.File->open()].
//!
//! @throws
//!   Throws an error if @[filename] couldn't be opened for writing.
//!
//! @returns
//!   Returns the number of bytes written.
//!
//! @seealso
//!   @[write_file()], @[read_bytes()], @[Stdio.File()->open()]
//!
int append_file(string filename, string str, int|void access)
{
  int ret;
  File f = File();

  if (query_num_arg() < 3) {
    access = 0666;
  }

  if(!f->open(filename, "awc", access))
    error("Couldn't open file "+filename+": " + strerror(f->errno()) + "\n");

  ret=f->write(str);
  f->close();
  return ret;
}
