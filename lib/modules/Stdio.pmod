#include <string.h>

inherit files;

class File
{
  inherit Fd_ref;

  mixed ___read_callback;
  mixed ___write_callback;
  mixed ___close_callback;
#if constant(__HAVE_OOB__)_
  mixed ___read_oob_callback;
  mixed ___write_oob_callback;
#endif
  mixed ___id;

  int open(string file, string mode,void|int bits)
  {
    _fd=Fd();
    if(query_num_arg()<3) bits=0666;
    return ::open(file,mode,bits);
  }

  int open_socket()
  {
    _fd=Fd();
    return ::open_socket();
  }

  int connect(string host, int port)
  {
    if(!_fd) _fd=Fd();
    return ::connect(host,port);
  }

  object(File) pipe(void|int how)
  {
    _fd=Fd();
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

  void create(mixed ... args)
  {
    if(sizeof(args))
    {
      switch(args[0])
      {
	case "stdin":
	  _fd=_stdin;
	  break;

	case "stdout":
	  _fd=_stdin;
	  break;

	case "stderr":
	  _fd=_stderr;
	  break;

	default:
	  open(@args);
      }
    }
  }

  static int do_assign(object to, object from)
  {
    if((program)Fd == (program)object_program(from))
    {
      to->_fd=from->dup();
    }else{
      to->_fd=from->_fd;
      to->___read_callback=from->___read_callback;
      to->___write_callback=from->___write_callback;
      to->___close_callback=from->___close_callback;
#if constant(__HAVE_OOB__)_
      to->___read_oob_callback=from->___read_oob_callback;
      to->___write_oob_callback=from->___write_oob_callback;
#endif
      to->___id=from->___id;
    }
    return 0;
  }

  int assign(object o)
  {
    return do_assign(this_object(), o);
  }

    object dup()
    {
      object o;
      o=File();
      do_assign(o,this_object());
      return o;
    }


  int close(void|string how)
  {
      if(::close(how||"rw"))
	_fd=0;
      return 1;
#if 0
    if(how)
    {
      if(::close(how))
	_fd=0;
    }else{
      _fd=0;
    }
#endif
  }

  static void my_read_callback()
  {
    string s=::read(8192,1);
    if(s && strlen(s))
    {
      ___read_callback(___id, s);
    }else{
      ::set_read_callback(0);
      ___close_callback(___id);
    }
  }

  static void my_write_callback() { ___write_callback(___id); }

#if constant(__HAVE_OOB__)_
  static void my_read_oob_callback()
  {
    string s=::read_oob(8192,1);
    if(s && strlen(s))
    {
      ___read_oob_callback(___id, s);
    }else{
      ___close_callback(___id);
    }
  }

  static void my_write_oob_callback() { ___write_oob_callback(___id); }
#endif

#define CBFUNC(X)				\
  void set_##X (mixed l##X)			\
  {						\
    ___##X=l##X;				\
    ::set_##X(l##X && my_##X);			\
  }						\
						\
  mixed query_##X ()				\
  {						\
    return ___##X;				\
  }

  CBFUNC(read_callback)
  CBFUNC(write_callback)
#if constant(__HAVE_OOB__)_
  CBFUNC(read_oob_callback)
  CBFUNC(write_oob_callback)
#endif

  mixed query_close_callback()  { return ___close_callback; }
  mixed set_close_callback(mixed c)  { ___close_callback=c; }
  void set_id(mixed i) { ___id=i; }
  void query_id() { return ___id; }

  void set_nonblocking(mixed|void rcb,
		       mixed|void wcb,
		       mixed|void ccb,
#if constant(__HAVE_OOB__)_
		       mixed|void roobcb,
		       mixed|void woobcb
#endif
    )
  {
    set_read_callback(rcb);
    set_write_callback(wcb);
    set_close_callback(ccb);
#if constant(__HAVE_OOB__)_
    set_read_oob_callback(roobcb);
    set_write_oob_callback(woobcb);
#endif
    ::set_nonblocking();
  }

  void set_blocking()
  {
    set_read_callback(0);
    set_write_callback(0);
    set_close_callback(0);
#if constant(__HAVE_OOB__)_
    set_read_oob_callback(0);
    set_write_oob_callback(0);
#endif
    ::set_blocking();
  }
};

class Port
{
  inherit port;
  object(File) accept()
  {
    if(object x=::accept())
    {
      object y=File();
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
    
    string read(int bytes)
    {
      while(strlen(b) - bpos < bytes)
	if(!get_data())
	  break;

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

int write_file(string filename,string what)
{
  int ret;
  object(File) f = File();

  if(!f->open(filename,"awc"))
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
  case "readline": return master()->resolv("readline");
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
  if(tmp->open(from,"r")) return 0;
  function r=tmp->read;
  tmp=File();
  if(tmp->open(to,"wct")) return 0;
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
