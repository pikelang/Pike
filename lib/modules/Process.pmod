#define error(X) throw( ({ (X), backtrace()[0..sizeof(backtrace())-2] }) )

import Stdio;

static private inherit File : file;

varargs int exec(string file,string ... foo)
{
  string path;
  if(search(file,"/"))
    return exece(combine_path(getcwd(),file),foo,getenv());

  path=getenv("PATH");

  foreach((path?(path/":"):({})),path)
    if(file_stat(path=combine_path(path,file)))
      return exece(path, foo,getenv());

  return 69;
}

varargs object spawn(string s,object stdin,object stdout,object stderr)
{
  if(object proc=fork())
  {
    return proc;
  }else{
    if(stdin) {
      stdin->dup2(File("stdin"));
      stdin->close();
      destruct(stdin);
    }

    if(stdout) {
      stdout->dup2(File("stdout"));
      stdout->close();
      destruct(stdout);
    }

    if(stderr) {
      stderr->dup2(File("stderr"));
      stderr->close();
      destruct(stderr);
    }
    ::close();
    exec("/bin/sh","-c",s);
    exit(69);
  }
}

string popen(string s)
{
  object p;
  string t;

  p=file::pipe();
  if(!p) error("Popen failed. (couldn't create pipe)\n");
  spawn(s,0,p,0);
  p->close();
  destruct(p);

  t=read();
  if(!t)
  {
    int e;
    e=errno();
    close();
    error("Popen failed with error "+e+".\n");
  }else{
    close();
  }
  return t;
}

int system(string s) { return spawn(s)->wait(); }
 
constant fork = predef::fork;
constant exece = predef::exece;


