#include <string.h>
object stderr=File("stderr");
#define error(X) throw( ({ (X), backtrace()[0..sizeof(backtrace())-2] }) )
program FILE = class {

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
      o=object_program(this_object())();
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

inherit "/precompiled/file";

string read_file(string filename,void|int start,void|int len)
{
  object buf,f;
  string ret,tmp;
  f=FILE();
  if(!f->open(filename,"r")) return 0;

  switch(query_num_arg())
  {
  case 1:
    ret=f->read(0x7fffffff);
    break;

  case 2:
    len=0x7fffffff;
  case 3:
    while(start-- && f->gets());
    buf=String_buffer();
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
  if(!open(filename,"r"))
    return 0;
  
  switch(query_num_arg())
  {
  case 1:
  case 2:
    len=0x7fffffff;
  case 3:
    seek(start);
  }
  ret=read(len);
  close();
  return ret;
}

int write_file(string filename,string what)
{
  int ret;
  if(!open(filename,"awc"))
    error("Couldn't open file "+filename+".\n");
  
  ret=write(what);
  close();
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
  stderr->write(s+": "+strerror(errno())+"\n");
#else
  stderr->write(s+": errno: "+errno()+"\n");
#endif
}

void create()
{
  add_constant("file_size",file_size);
  add_constant("write_file",write_file);
  add_constant("read_bytes",read_bytes);
  add_constant("read_file",read_file);
  add_constant("perror",perror);


  master()->add_precompiled_program("/precompiled/FILE", FILE);

  add_constant("stdin",FILE("stdin"));
  add_constant("stdout",File("stdout"));
  add_constant("stderr",stderr);
}
