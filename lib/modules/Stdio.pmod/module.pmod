// $Id: module.pmod,v 1.57 1999/07/15 17:36:21 mirar Exp $

import String;

inherit files;

class File
{
  inherit Fd_ref;

  mixed ___read_callback;
  mixed ___write_callback;
  mixed ___close_callback;
#if constant(files.__HAVE_OOB__)
  mixed ___read_oob_callback;
  mixed ___write_oob_callback;
#endif
  mixed ___id;

#ifdef __STDIO_DEBUG
  string __closed_backtrace;
#define CHECK_OPEN()								\
    if(!_fd)									\
    {										\
      throw(({									\
	"Stdio.File(): line "+__LINE__+" on closed file.\n"+			\
	  (__closed_backtrace ? 						\
	   sprintf("File was closed from:\n    %-=200s\n",__closed_backtrace) :	\
	   "This file has never been open.\n" ),				\
	  backtrace()}));							\
      										\
    }
#else
#define CHECK_OPEN()
#endif

  int errno()
  {
    return _fd && ::errno();
  }

  int open(string file, string mode, void|int bits)
  {
    _fd=Fd();
#ifdef __STDIO_DEBUG
    __closed_backtrace=0;
#endif
    if(query_num_arg()<3) bits=0666;
    return ::open(file,mode,bits);
  }

  int open_socket(int|void port, string|void address)
  {
    _fd=Fd();
#ifdef __STDIO_DEBUG
    __closed_backtrace=0;
#endif
    switch(query_num_arg()) {
    case 0:
      return ::open_socket();
    case 1:
      return ::open_socket(port);
    default:
      return ::open_socket(port, address);
    }
  }

  int connect(string host, int port)
  {
    if(!_fd) _fd=Fd();
#ifdef __STDIO_DEBUG
    __closed_backtrace=0;
#endif
    return ::connect(host,port);
  }

  static private function(int, mixed ...:void) _async_cb;
  static private array(mixed) _async_args;
  static private void _async_connected(mixed|void ignored)
  {
    // Copy the args to avoid races.
    function(int, mixed ...:void) cb = _async_cb;
    array(mixed) args = _async_args;
    _async_cb = 0;
    _async_args = 0;
    set_nonblocking(0,0,0);
    cb(1, @args);
  }
  static private void _async_failed(mixed|void ignored)
  {
    // Copy the args to avoid races.
    function(int, mixed ...:void) cb = _async_cb;
    array(mixed) args = _async_args;
    _async_cb = 0;
    _async_args = 0;
    set_nonblocking(0,0,0);
    cb(0, @args);
  }
  // NOTE: Zaps nonblocking-state.
  int async_connect(string host, int port,
		    function(int, mixed ...:void) callback,
		    mixed ... args)
  {
    if (!_fd && !open_socket()) {
      // Out of sockets?
      return 0;
    }
    _async_cb = callback;
    _async_args = args;
    set_nonblocking(0, _async_connected, _async_failed);
    mixed err;
    int res;
    if (err = catch(res = connect(host, port))) {
      // Illegal format. -- Bad hostname?
      call_out(_async_failed, 0);
    } else if (!res) {
      // Connect failed.
      call_out(_async_failed, 0);
    }
    return(1);	// OK so far. (Or rather the callback will be used).
  }

  object(File) pipe(void|int how)
  {
    _fd=Fd();
#ifdef __STDIO_DEBUG
    __closed_backtrace=0;
#endif
    if(query_num_arg()==0)
      how=PROP_NONBLOCK | PROP_BIDIRECTIONAL;
    if(object(Fd) fd=::pipe(how))
    {
      object o=File();
      o->_fd=fd;
      return o;
    }else{
      return 0;
    }
  }

  void create(string|void file,void|string mode,void|int bits)
  {
    switch(file)
    {
      case "stdin":
	_fd=_stdin;
#ifdef __STDIO_DEBUG
	__closed_backtrace=0;
#endif
      case 0:
	break;
	
      case "stdout":
	_fd=_stdout;
#ifdef __STDIO_DEBUG
	__closed_backtrace=0;
#endif
	break;
	
      case "stderr":
	_fd=_stderr;
#ifdef __STDIO_DEBUG
	__closed_backtrace=0;
#endif
	break;
	
      default:
	_fd=Fd();
#ifdef __STDIO_DEBUG
	__closed_backtrace=0;
#endif
	if(query_num_arg()<3) bits=0666;
	::open(file,mode,bits);
    }
  }

  int assign(object o)
  {
    if((program)Fd == (program)object_program(o))
    {
      _fd = o->dup();
    }else{
      _fd = o->_fd;
      if(___read_callback = o->___read_callback)
	_fd->_read_callback=__stdio_read_callback;

      if(___write_callback = o->___write_callback)
	_fd->_write_callback=__stdio_write_callback;

      ___close_callback = o->___close_callback;
#if constant(files.__HAVE_OOB__)
      if(___read_oob_callback = o->___read_oob_callback)
	_fd->_read_oob_callback = __stdio_read_oob_callback;

      if(___write_oob_callback = o->___write_oob_callback)
	_fd->_write_oob_callback = __stdio_write_oob_callback;
#endif
      ___id = o->___id;
      
    }
    return 0;
  }

  object dup()
  {
    object to = File();
    to->_fd = _fd;
    if(to->___read_callback = ___read_callback)
      _fd->_read_callback=to->__stdio_read_callback;

    if(to->___write_callback = ___write_callback)
      _fd->_write_callback=to->__stdio_write_callback;

    to->___close_callback = ___close_callback;
#if constant(files.__HAVE_OOB__)
    if(to->___read_oob_callback = ___read_oob_callback)
      _fd->_read_oob_callback=to->__stdio_read_oob_callback;

    if(to->___write_oob_callback = ___write_oob_callback)
      _fd->_write_oob_callback=to->__stdio_write_oob_callback;
#endif
    to->___id = ___id;
    return to;
  }


  int close(void|string how)
  {
    if(::close(how||"rw"))
    {
#define FREE_CB(X) if(___##X && query_##X == __stdio_##X) ::set_##X(0)
      FREE_CB(read_callback);
      FREE_CB(write_callback);
#if constant(files.__HAVE_OOB__)
      FREE_CB(read_oob_callback);
      FREE_CB(write_oob_callback);
#endif
      _fd=0;
#ifdef __STDIO_DEBUG
      __closed_backtrace=master()->describe_backtrace(backtrace());
#endif
    }
    return 1;
  }

  // FIXME: No way to specify the maximum to read.
  static void __stdio_read_callback()
  {
#if defined(__STDIO_DEBUG) && !defined(__NT__)
    if(!::peek())
      throw( ({"Read callback with no data to read!\n",backtrace()}) );
#endif
    string s=::read(8192,1);
    if(s && strlen(s))
    {
      ___read_callback(___id, s);
    }else{
      ::set_read_callback(0);
      if (___close_callback) {
	___close_callback(___id);
      }
    }
  }

  static void __stdio_write_callback() { ___write_callback(___id); }

#if constant(files.__HAVE_OOB__)
  static void __stdio_read_oob_callback()
  {
    string s=::read_oob(8192,1);
    if(s && strlen(s))
    {
      ___read_oob_callback(___id, s);
    }else{
      ::set_read_oob_callback(0);
      if (___close_callback) {
	___close_callback(___id);
      }
    }
  }

  static void __stdio_write_oob_callback() { ___write_oob_callback(___id); }
#endif

#define SET(X,Y) ::set_##X ((___##X = (Y)) && __stdio_##X)
#define _SET(X,Y) _fd->_##X=(___##X = (Y)) && __stdio_##X

#define CBFUNC(X)					\
  void set_##X (mixed l##X)				\
  {							\
    CHECK_OPEN();                                       \
    SET( X , l##X );					\
  }							\
							\
  mixed query_##X ()					\
  {							\
    return ___##X;					\
  }

  CBFUNC(read_callback)
  CBFUNC(write_callback)
#if constant(files.__HAVE_OOB__)
  CBFUNC(read_oob_callback)
  CBFUNC(write_oob_callback)
#endif

  mixed query_close_callback()  { return ___close_callback; }
  mixed set_close_callback(mixed c)  { ___close_callback=c; }
  void set_id(mixed i) { ___id=i; }
  mixed query_id() { return ___id; }

  void set_nonblocking(mixed|void rcb,
		       mixed|void wcb,
		       mixed|void ccb,
#if constant(files.__HAVE_OOB__)
		       mixed|void roobcb,
		       mixed|void woobcb
#endif
    )
  {
    CHECK_OPEN();
    ::_disable_callbacks(); // Thread safing

    _SET(read_callback,rcb);
    _SET(write_callback,wcb);
    ___close_callback=ccb;

#if constant(files.__HAVE_OOB__)
    _SET(read_oob_callback,roobcb);
    _SET(write_oob_callback,woobcb);
#endif
#ifdef __STDIO_DEBUG
    if(mixed x=catch { ::set_nonblocking(); })
    {
      x[0]+=(__closed_backtrace ? 
	   sprintf("File was closed from:\n    %-=200s\n",__closed_backtrace) :
	   "This file has never been open.\n" )+
	(_fd?"_fd is nonzero\n":"_fd is zero\n");
      throw(x);
    }
#else
    ::set_nonblocking();
#endif
    ::_enable_callbacks();

  }

  void set_blocking()
  {
    CHECK_OPEN();
    ::_disable_callbacks(); // Thread safing
    SET(read_callback,0);
    SET(write_callback,0);
    ___close_callback=0;
#if constant(files.__HAVE_OOB__)
    SET(read_oob_callback,0);
    SET(write_oob_callback,0);
#endif
    ::set_blocking();
    ::_enable_callbacks();
  }

  void destroy()
  {
    if(_fd)
    { 
      FREE_CB(read_callback);
      FREE_CB(write_callback);
#if constant(files.__HAVE_OOB__)
      FREE_CB(read_oob_callback);
      FREE_CB(write_oob_callback);
#endif
    }
  }
};

class Port
{
  inherit _port;
  object(File) accept()
  {
    if(object x=::accept())
    {
      object(File) y=File();
      y->_fd=x;
      return y;
    }
    return 0;
  }
}

object stderr=File("stderr");
object stdout=File("stdout");

#define error(X) throw( ({ (X), backtrace()[0..sizeof(backtrace())-2] }) )
class FILE {

#define BUFSIZE 16384
    inherit File : file;

    /* Private functions / buffers etc. */

    private string b="";
    private int bpos=0;

    inline private static nomask int get_data()
    {
      string s;
      b = b[bpos .. ];
      bpos=0;
      s = file::read(BUFSIZE,1);
      if(!s || !strlen(s))
	return 0;
      b += s;
      return 1;
    }

    inline private static nomask string extract(int bytes, int|void skip)
    {
      string s;
      s=b[bpos..bpos+bytes-1];
      bpos += bytes+skip;
      return s;
    }


    /* Public functions. */
    string gets()
    {
      int p,tmp=0;
      while((p=search(b, "\n", bpos+tmp)) == -1)
      {
	tmp=strlen(b)-bpos;
	if(!get_data()) return 0;
      }
      return extract(p-bpos, 1);
    }

    int seek(int pos)
    {
      bpos=0;
      b="";
      return file::seek(pos);
    }

    int tell()
    {
      return file::tell()-sizeof(b)+bpos;
    }

    int close(void|string mode)
    {
      bpos=0;
      b="";
      if(!mode) mode="rw";
      file::close(mode);
    }

    int open(string file, void|string mode)
    {
      bpos=0;
      b="";
      if(!mode) mode="rwc";
      return file::open(file,mode);
    }

    int open_socket()
    {
      bpos=0;
      b="";
      return file::open_socket();
    }

    object pipe()
    {
      bpos=0;
      b="";
      return file::pipe();
    }

    int assign(object foo)
    {
      bpos=0;
      b="";
      return ::assign(foo);
    }

  object(FILE) dup()
  {
    object(FILE) o=FILE();
    o->assign(this_object());
    return o;
  }

    void set_nonblocking()
    {
      error("Cannot use nonblocking IO with buffered files.\n");
    }

    int printf(string fmt, mixed ... data)
    {
      if(!sizeof(data))
	return file::write(fmt);
      else
	return file::write(sprintf(fmt,@data));
    }
    
    string read(int|void bytes,void|int(0..1) now)
    {
      if (!query_num_arg()) {
	bytes = 0x7fffffff;
      }
      while(strlen(b) - bpos < bytes)
	if(!get_data()) {
	  // EOF.
	  string res = b[bpos..];
	  b = "";
	  bpos = 0;
	  return res;
	}
	else if (now) break;

      return extract(bytes);
    }

    string ungets(string s)
    {
      b=s+b[bpos..];
      bpos=0;
    }

    int getchar()
    {
      if(strlen(b) - bpos < 1)
	if(!get_data())
	  return -1;

      return b[bpos++];
    }
  };

object(FILE) stdin=FILE("stdin");

string read_file(string filename,void|int start,void|int len)
{
  object(FILE) f;
  string ret, tmp;
  f=FILE();
  if(!f->open(filename,"r")) return 0;

  // Disallow devices and directories.
  array st;
  if (f->stat && (st = f->stat()) && (st[1] < 0)) {
    throw(({ sprintf("Stdio.read_file(): File \"%s\" is not a regular file!\n",
		     filename),
	     backtrace()
    }));
  }

  switch(query_num_arg())
  {
  case 1:
    ret=f->read(0x7fffffff);
    break;

  case 2:
    len=0x7fffffff;
  case 3:
    while(start-- && f->gets());
    object(String_buffer) buf=String_buffer();
    while(len-- && (tmp=f->gets()))
    {
      buf->append(tmp);
      buf->append("\n");
    }
    ret=buf->get_buffer();
    destruct(buf);
  }
  destruct(f);

  return ret;
}

string read_bytes(string filename,void|int start,void|int len)
{
  string ret;
  object(File) f = File();

  if(!f->open(filename,"r"))
    return 0;
  
  // Disallow devices and directories.
  array st;
  if (f->stat && (st = f->stat()) && (st[1] < 0)) {
    throw(({sprintf("Stdio.read_bytes(): File \"%s\" is not a regular file!\n",
		    filename),
	    backtrace()
    }));
  }

  switch(query_num_arg())
  {
  case 1:
  case 2:
    len=0x7fffffff;
  case 3:
    if(start)
      f->seek(start);
  }
  ret=f->read(len);
  f->close();
  return ret;
}

int write_file(string filename, string what, int|void access)
{
  int ret;
  object(File) f = File();

  if (query_num_arg() < 3) {
    access = 0666;
  }

  if(!f->open(filename, "twc", access))
    error("Couldn't open file "+filename+".\n");
  
  ret=f->write(what);
  f->close();
  return ret;
}

int append_file(string filename, string what, int|void access)
{
  int ret;
  object(File) f = File();

  if (query_num_arg() < 3) {
    access = 0666;
  }

  if(!f->open(filename, "awc", access))
    error("Couldn't open file "+filename+".\n");
  
  ret=f->write(what);
  f->close();
  return ret;
}

int file_size(string s)
{
  int *stat;
  stat=file_stat(s);
  if(!stat) return -1;
  return stat[1]; 
}

void perror(string s)
{
#if efun(strerror)
  stderr->write(s+": "+strerror(predef::errno())+"\n");
#else
  stderr->write(s+": errno: "+predef::errno()+"\n");
#endif
}

mixed `[](string index)
{
  mixed x=`->(this_object(),index);
  if(x) return x;
  switch(index)
  {
  case "readline": return master()->resolv("Stdio")["Readline"]->readline;
  default: return ([])[0];
  }
}

#if constant(system.cp)
constant cp=system.cp;
#else
#define BLOCK 65536
int cp(string from, string to)
{
  string data;
  object tmp=File();
  if(!tmp->open(from,"r")) return 0;
  function r=tmp->read;
  tmp=File();
  if(!tmp->open(to,"wct")) return 0;
  function w=tmp->write;
  do
  {
    data=r(BLOCK);
    if(!data) return 0;
    if(w(data)!=strlen(data)) return 0;
  }while(strlen(data) == BLOCK);

  return 1;
}
#endif

/*
 * Asynchronous sending of files.
 */

#define READER_RESTART 4
#define READER_HALT 32

// FIXME: Support for timeouts?
static class nb_sendfile
{
  static object from;
  static int len;
  static array(string) trailers;
  static object to;
  static function(int, mixed ...:void) callback;
  static array(mixed) args;

  // NOTE: Always modified from backend callbacks, so no need
  // for locking.
  static array(string) to_write = ({});
  static int sent;

  static int reader_awake;
  static int writer_awake;

  static int blocking_to;
  static int blocking_from;

  /* Reader */

  static void reader_done()
  {
#ifdef SENDFILE_DEBUG
    werror("Stdio.sendfile(): Reader done.\n");
#endif /* SENDFILE_DEBUG */

    from->set_nonblocking(0,0,0);
    from = 0;
    if (trailers) {
      to_write += trailers;
    }
    if (blocking_to) {
      while(sizeof(to_write)) {
	if (!do_write()) {
	  // Connection closed or Disk full.
	  writer_done();
	  return;
	}
      }
      if (!from) {
	writer_done();
	return;
      }
    } else {
      if (sizeof(to_write)) {
	start_writer();
      } else {
	writer_done();
	return;
      }
    }
  }

  static void close_cb(mixed ignored)
  {
#ifdef SENDFILE_DEBUG
    werror("Stdio.sendfile(): Input EOF.\n");
#endif /* SENDFILE_DEBUG */

    reader_done();
  }

  static void do_read()
  {
#ifdef SENDFILE_DEBUG
    werror("Stdio.sendfile(): Blocking read.\n");
#endif /* SENDFILE_DEBUG */

    string more_data = from->read(65536, 1);
    if (more_data == "") {
      // EOF.
#ifdef SENDFILE_DEBUG
      werror("Stdio.sendfile(): Blocking read got EOF.\n");
#endif /* SENDFILE_DEBUG */

      from = 0;
      if (trailers) {
	to_write += (trailers - ({ "" }));
	trailers = 0;
      }
    } else {
      to_write += ({ more_data });
    }
  }

  static void read_cb(mixed ignored, string data)
  {
#ifdef SENDFILE_DEBUG
    werror("Stdio.sendfile(): Read callback.\n");
#endif /* SENDFILE_DEBUG */
    if (len > 0) {
      if (sizeof(data) < len) {
	len -= sizeof(data);
	to_write += ({ data });
      } else {
	to_write += ({ data[..len-1] });
	reader_done();
	return;
      }
    } else {
      to_write += ({ data });
    }
    if (blocking_to) {
      while(sizeof(to_write)) {
	if (!do_write()) {
	  // Connection closed or Disk full.
	  writer_done();
	  return;
	}
      }
      if (!from) {
	writer_done();
	return;
      }
    } else {
      if (sizeof(to_write) > READER_HALT) {
	// Go to sleep.
	from->set_nonblocking(0,0,0);
	reader_awake = 0;
      }
      start_writer();
    }
  }

  static void start_reader()
  {
#ifdef SENDFILE_DEBUG
    werror("Stdio.sendfile(): Starting the reader.\n");
#endif /* SENDFILE_DEBUG */
    if (!reader_awake) {
      reader_awake = 1;
      from->set_nonblocking(read_cb, 0, close_cb);
    }
  }

  /* Writer */

  static void writer_done()
  {
#ifdef SENDFILE_DEBUG
    werror("Stdio.sendfile(): Writer done.\n");
#endif /* SENDFILE_DEBUG */

    // Disable any reader.
    if (from) {
      from->set_nonblocking(0,0,0);
    }

    // Disable any writer.
    if (to) {
      to->set_nonblocking(0,0,0);
    }

    // Make sure we get rid of any references...
    to_write = 0;
    trailers = 0;
    from = 0;
    to = 0;
    array(mixed) a = args;
    function(int, mixed ...:void) cb = callback;
    args = 0;
    callback = 0;
    if (cb) {
      cb(sent, @a);
    }
  }

  static int do_write()
  {
#ifdef SENDFILE_DEBUG
    werror("Stdio.sendfile(): Blocking writer.\n");
#endif /* SENDFILE_DEBUG */

    int bytes = to->write(to_write);

    if (bytes > 0) {
      sent += bytes;

      int n;
      for (n = 0; n < sizeof(to_write); n++) {
	if (bytes < sizeof(to_write[n])) {
	  to_write[n] = to_write[n][bytes..];
	  to_write = to_write[n..];

	  return 1;
	} else {
	  bytes -= sizeof(to_write[n]);
	  if (!bytes) {
	    to_write = to_write[n+1..];
	    return 1;
	  }
	}
      }
      // Not reached, but...
      return 1;
    } else {
#ifdef SENDFILE_DEBUG
      werror("Stdio.sendfile(): Blocking writer got EOF.\n");
#endif /* SENDFILE_DEBUG */
      // Premature end of file!
      return 0;
    }
  }

  static void write_cb(mixed ignored)
  {
#ifdef SENDFILE_DEBUG
    werror("Stdio.sendfile(): Write callback.\n");
#endif /* SENDFILE_DEBUG */
    if (do_write()) {
      if (from) {
	if (sizeof(to_write) < READER_RESTART) {
	  if (blocking_from) {
	    do_read();
	    if (!sizeof(to_write)) {
	      // Done.
	      writer_done();
	    }
	  } else {
	    if (!sizeof(to_write)) {
	      // Go to sleep.
	      to->set_nonblocking(0,0,0);
	      writer_awake = 0;
	    }
	    start_reader();
	  }
	}
      } else if (!sizeof(to_write)) {
	// Done.
	writer_done();
      }
    } else {
      // Premature end of file!
      writer_done();
    }
  }

  static void start_writer()
  {
#ifdef SENDFILE_DEBUG
    werror("Stdio.sendfile(): Starting the writer.\n");
#endif /* SENDFILE_DEBUG */

    if (!writer_awake) {
      writer_awake = 1;
      to->set_nonblocking(0, write_cb, 0);
    }
  }

  /* Blocking */
  static void do_blocking()
  {
#ifdef SENDFILE_DEBUG
    werror("Stdio.sendfile(): Blocking I/O.\n");
#endif /* SENDFILE_DEBUG */

    if (from && (sizeof(to_write) < READER_RESTART)) {
      do_read();
    }
    if (sizeof(to_write) && do_write()) {
      call_out(do_blocking, 0);
    } else {
#ifdef SENDFILE_DEBUG
      werror("Stdio.sendfile(): Blocking I/O done.\n");
#endif /* SENDFILE_DEBUG */
      // Done.
      from = 0;
      to = 0;
      writer_done();
    }
  }

  /* Starter */

  void create(array(string) hd,
	      object f, int off, int l,
	      array(string) tr,
	      object t,
	      function(int, mixed ...:void)|void cb,
	      mixed ... a)
  {
    if (!l) {
      // No need for from.
      f = 0;

      // No need to differentiate between headers and trailers.
      if (tr) {
	if (hd) {
	  hd += tr;
	} else {
	  hd = tr;
	}
	tr = 0;
      }
    }

    if (!f && (!hd || !sizeof(hd - ({ "" })))) {
      // NOOP!
#ifdef SENDFILE_DEBUG
      werror("Stdio.sendfile(): NOOP!\n");
#endif /* SENDFILE_DEBUG */
      call_out(cb, 0, 0, @args);
      return;
    }

    to_write = (hd || ({})) - ({ "" });
    from = f;
    len = l;
    trailers = (tr || ({})) - ({ "" });
    to = t;
    callback = cb;
    args = a;

    blocking_to = ((!to->set_nonblocking) ||
		   (to->mode && !(to->mode() & PROP_NONBLOCK)));

    if (blocking_to && to->set_blocking) {
#ifdef SENDFILE_DEBUG
      werror("Stdio.sendfile(): Blocking to.\n");
#endif /* SENDFILE_DEBUG */
      to->set_blocking();
    }

    if (from) {
      blocking_from = ((!from->set_nonblocking) ||
		       (from->mode && !(from->mode() & PROP_NONBLOCK)));
	
      if (off >= 0) {
	from->seek(off);
      }
      if (blocking_from) {
#ifdef SENDFILE_DEBUG
	werror("Stdio.sendfile(): Blocking from.\n");
#endif /* SENDFILE_DEBUG */
	if (from->set_blocking) {
	  from->set_blocking();
	}
      } else {
#ifdef SENDFILE_DEBUG
	werror("Stdio.sendfile(): Starting reader.\n");
#endif /* SENDFILE_DEBUG */
	start_reader();
      }
    }

    if (blocking_to) {
      if (!from || blocking_from) {
	// Can't use the reader to push data.

	// Could have a direct call to do_blocking here,
	// but then the callback would be called from the wrong context.
#ifdef SENDFILE_DEBUG
	werror("Stdio.sendfile(): Using fully blocking I/O.\n");
#endif /* SENDFILE_DEBUG */
	call_out(do_blocking, 0);
      }
    } else {
      if (blocking_from) {
#ifdef SENDFILE_DEBUG
	werror("Stdio.sendfile(): Reading some data.\n");
#endif /* SENDFILE_DEBUG */
	do_read();
	if (!sizeof(to_write)) {
#ifdef SENDFILE_DEBUG
	  werror("Stdio.sendfile(): NOOP!\n");
#endif /* SENDFILE_DEBUG */
	  call_out(cb, 0, 0, @args);
	}
      }
      if (sizeof(to_write)) {
#ifdef SENDFILE_DEBUG
	werror("Stdio.sendfile(): Starting the writer.\n");
#endif /* SENDFILE_DEBUG */
	start_writer();
      }
    }
  }
}

// FIXME: Support for timeouts?
object sendfile(array(string) headers,
		object from, int offset, int len,
		array(string) trailers,
		object to,
		function(int, mixed ...:void)|void cb,
		mixed ... args)
{
#if constant(files.sendfile)
  // Try using files.sendfile().
  
  mixed err = catch {
    return files.sendfile(headers, from, offset, len,
			  trailers, to, cb, @args);
  };

#ifdef SENDFILE_DEBUG
  werror(sprintf("files.sendfile() failed:\n%s\n",
		 describe_backtrace(err)));
#endif /* SENDFILE_DEBUG */

#endif /* files.sendfile */

  // Use nb_sendfile instead.
  return nb_sendfile(headers, from, offset, len, trailers, to, cb, @args);
}


class UDP
{
   inherit files.UDP;

   private static array extra=0;
   private static function callback=0;

   object set_nonblocking(mixed ...stuff)
   {
      if (stuff!=({})) 
	 set_read_callback(@stuff);
      return _set_nonblocking();
   }

   object set_read_callback(function f,mixed ...ext)
   {
      extra=ext;
      callback=f;
      _set_read_callback(_read_callback);
      return this_object();
   }
   
   private static void _read_callback()
   {
      mapping i;
      while (i=read())
	 callback(i,@extra);
   }
}
