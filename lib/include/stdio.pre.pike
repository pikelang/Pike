inherit "/precompiled/file";
#define error(X) throw( ({ (X), backtrace()[0..sizeof(backtrace())-2] }) )

void perror(string s)
{
#if efun(strerror)
  write(s+": "+strerror(errno())+"\n");
#else
  write(s+": errno: "+errno()+"\n");
#endif
}

void create()
{
  file::create("stderr");
  add_constant("perror",perror);


  master()->add_precompiled_program("/precompiled/FILE", class {

#define BUFSIZE 16384
    inherit "/precompiled/file";

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
      int p;
      while((p=search(b, "\n", bpos)) == -1)
	if(!get_data())
	  return 0;
      return extract(p-bpos, 1);
    }

    int seek(int pos)
    {
      bpos=0;
      b="";
      return file::seek(pos);
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

    object dup()
    {
      object o;
      o=clone(object_program(this_object()));
      o->assign(file::dup());
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
	  return 0;

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
  });
}
