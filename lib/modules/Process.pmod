#define error(X) throw( ({ (X), backtrace()[0..sizeof(backtrace())-2] }) )

import Stdio;

// static private inherit File : file;

varargs int exec(string file,string ... foo)
{
  if (sizeof(file)) {
    string path;

    if(search(file,"/") >= 0)
      return exece(combine_path(getcwd(),file),foo,getenv());

    path=getenv("PATH");

    foreach(path ? path/":" : ({}) , path)
      if(file_stat(path=combine_path(path,file)))
	return exece(path, foo,getenv());
  }
  return 69;
}

varargs int spawn(string s,object stdin,object stdout,object stderr,
		  function|void cleanup, mixed ... args)
{
  int pid;

  pid=fork();
  
  if(pid==-1)
    error("No more processes.\n");

  if(pid)
  {
    return pid;
  }else{
    if(stdin ) stdin ->dup2(File("stdin"));
    if(stdout) stdout->dup2(File("stdout"));
    if(stderr) stderr->dup2(File("stderr"));

    if(stdin ) destruct(stdin);
    if(stdout) destruct(stdout);
    if(stderr) destruct(stderr);

    if (cleanup) {
      cleanup(@args);
    }

    exec("/bin/sh","-c",s);
    exit(69);
  }
}

string popen(string s)
{
  object p;
  string t;
  object f = File();

  if (!f) error("Popen failed. (couldn't create pipe)\n");

  p=f->pipe();
  if(!p) error("Popen failed. (couldn't create pipe)\n");
  spawn(s,0,p,0, destruct, f);
  p->close();
  destruct(p);

  t=f->read(0x7fffffff);
  if(!t)
  {
    int e;
    e=f->errno();
    f->close();
    destruct(f);
    error("Popen failed with error "+e+".\n");
  } else {
    f->close();
    destruct(f);
  }
  return t;
}

void system(string s)
{
  object p;
  int pid;
  string t;
  object f;

  f = File();
  if (!f) error("System failed.\n");

  p=f->pipe();
  if(!p) error("System() failed.\n");
  p->set_close_on_exec(0);
  if(pid=fork())
  {
    p->close();
    destruct(p);
    /* Nothing will ever be written here, we are just waiting for it
     * to close
     */
    f->read(1);
  }else{
    f->close();
    destruct(f);
    exec("/bin/sh","-c",s);
    exit(69);
  }
}
 
constant fork = predef::fork;
constant exece = predef::exece;


