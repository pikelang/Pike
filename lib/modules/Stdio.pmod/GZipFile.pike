// $Id: GZipFile.pike,v 1.2 2002/08/26 09:08:33 grendel Exp $
#pike __REAL_VERSION__

//! Allows the user to open a Gzip archive and read and write
//! it's contents in an uncompressed form.
//! @note
//! An important limitation on this class is that it may only be used
//! for reading @b{or@} writing, not both at the same time.
//! Please also note that if you want to reopen a file for reading
//! after a write, you must close the file before calling open or
//! strange effects might be the result.
inherit Gz._file;

private int is_open;

//! @decl void create(void|string|int file, void|string mode)
//! @param file
//! Filename or filedescriptor of the gzip file to open.
//! @param mode
//! mode for the file. Defaults to "rb".
//! @seealso 
//!  [open]
void create(mixed ... args) {
  ::create();

  if (sizeof(args))
    open(@args);
}

void destroy() {
  close();
}

//! @param file
//! Filename or filedescriptor of the gzip file to open.
//! @param mode
//! mode for the file. Defaults to "rb".
//! May be one of the following:
//! @dl
//! @item rb
//! read mode
//! @item wb
//! write mode
//! @item ab
//! append mode
//! @enddl
//! For the wb and ab mode, additional parameters may
//! be specified. Please se zlib manual for more info.
//! @returns 
//!  non-zero if successful.
int open(string|int file, void|string mode) {
  string open_mode="rb";

  if (is_open)
    ::close();

  if (stringp(mode)) {
    open_mode = lower_case(mode);
  }

  is_open = ::open(file, open_mode);
  return is_open;
}

//! Reads data from the file.
//! If no argument is given, the whole file is read.
int|string read(void|int length) {
  string ret = "";

  if (!is_open)
    return 0;

  if (length)
    return ::read(length);
  else
    while(!eof()) {
      ret += ::read(1024*64);
    }
  return ret;
}
