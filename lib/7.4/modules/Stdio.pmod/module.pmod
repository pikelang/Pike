// $Id: module.pmod,v 1.4 2004/05/04 00:11:38 nilsson Exp $
#pike 7.5

//! The life length of the Fd instance has changed. In newer versions
//! it's always the same as for the File instance. That can
//! theoretically cause incompatibilities in code that uses File.dup
//! and File.assign, which are deprecated and very rarely used.
class File
{
  inherit Stdio.File;

  int is_open()
  {
    return _fd && ::is_open();
  }

  int errno()
  {
    return _fd && ::errno();
  }

  int open(string file, string mode, void|int bits)
  {
    _fd=Stdio.Fd();
    return ::open(file,mode,bits);
  }

#if constant(files.__HAVE_OPENPT__)
  int openpt(string mode)
  {
    _fd=Stdio.Fd();
    return ::openpt(mode);
  }
#endif

  int open_socket(int|string... args)
  {
    _fd=Stdio.Fd();
    return ::open_socket (@args);
  }

  int connect(string host, int|string port,
	      void|string client, void|int|string client_port)
  {
    if(!_fd) _fd=Stdio.Fd();
    return ::connect(host, port, client, client_port);
  }

#if constant(files.__HAVE_CONNECT_UNIX__)
  int connect_unix(string path)
  {
    if(!_fd) _fd=Stdio.Fd();
    return ::connect_unix( path );
  }
#endif

  File pipe(void|int required_properties)
  {
    _fd=Stdio.Fd();
    if (zero_type (required_properties))
      return ::pipe();
    else
      return ::pipe (required_properties);
  }

  static void create(int|string... args)
  {
    ::create (@args);
    if (!sizeof (args)) _fd = 0;
  }

  int close(void|string how)
  {
    if (::close (how)) {
      _fd = 0;
      return 1;
    }
    return 0;
  }
}
