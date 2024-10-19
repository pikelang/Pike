#pike __REAL_VERSION__
#require constant(_Gz)

#define DATA_CHUNK_SIZE 64*1024

//! Allows the user to open a Gzip archive and read and write
//! it's contents in an uncompressed form, emulating the @[Stdio.File]
//! interface.
//! @note
//! An important limitation on this class is that it may only be used
//! for reading @b{or@} writing, not both at the same time.
//! Please also note that if you want to reopen a file for reading
//! after a write, you must close the file before calling open or
//! strange effects might be the result.
inherit ._file;

private int is_open = 0;

//! @decl void create(void|string|int|Stdio.Stream file, void|string mode)
//! @param file
//! Filename or filedescriptor of the gzip file to open, or an already
//! open Stream.
//! @param mode
//! mode for the file. Defaults to "rb".
//! @seealso
//!   @[open] @[Stdio.File]
protected void create(mixed ... args) {
  ::create(@args);

  if(sizeof(args)) {
    is_open = 1;
  }
}

protected string _sprintf(int t)
{
  switch(t) {
  case 'O':
    return sprintf("Gz.File(/*%s open */)", is_open ? "" : " not");
  case 't':
    return "Gz.File";
  default:
    return UNDEFINED;
  }
}

int close()
{
  is_open = 0;
  return ::close();
}

protected void _destruct() {
  close();
}

//! @param file
//! Filename or filedescriptor of the gzip file to open, or an already
//! open Stream.
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
int open(string|int|Stdio.Stream file, void|string mode) {
  string open_mode="rb";
  if (is_open) {
    ::close();
  }

  if (stringp(mode)) {
    open_mode = lower_case(mode);
  }

  is_open = ::open(file, open_mode);
  return is_open;
}

//! Reads data from the file.
//! If no argument is given, the whole file is read.
int|string read(void|int length) {
  if (!is_open) {
    return 0;
  }

  if (!zero_type(length))
    return ::read(length);

  String.Buffer buf = String.Buffer();
  string data;
  do {
    if (!(data = ::read(DATA_CHUNK_SIZE))) break;
    buf->add(data);
  } while (sizeof(data));
  return (string)buf;
}


function(:string) read_function(int nbytes)
//! Returns a function that when called will call @[read] with
//! nbytes as argument. Can be used to get various callback
//! functions, eg for the fourth argument to
//! @[String.SplitIterator].
{
  return lambda(){ return ::read(nbytes); };
}

String.SplitIterator|Stdio.LineIterator line_iterator( int|void trim )
//! Returns an iterator that will loop over the lines in this file.
//! If trim is true, all @tt{'\r'@} characters will be removed from
//! the input.
{
  if( trim )
    return String.SplitIterator( "",(<'\n','\r'>),1,
      			   read_function(DATA_CHUNK_SIZE));
  // This one is about twice as fast, but it's way less flexible.
  return Stdio.LineIterator( read_function(DATA_CHUNK_SIZE) );
}
