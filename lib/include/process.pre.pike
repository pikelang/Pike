#define error(X) throw( ({ (X), backtrace()[0..sizeof(backtrace())-2] }) )
inherit "/precompiled/file";

varargs int exec(string file,string ... foo)
{
  string path;
  if(search(file,"/"))
    return exece(combine_path(getcwd(),file),foo,getenv());

  path=getenv("PATH");

  foreach(path/":",path)
    if(file_stat(path=combine_path(path,file)))
      return exece(path, foo,getenv());

  return 69;
}

void int spawn(string s,object stdin,object stdout,object stderr)
{
  object p;
  int pid;
  string t;

  pid=fork();
  
  if(pid==-1)
    error("No more processes.\n");

  if(pid)
  {
    return pid;
  }else{
    if(stdin)
      stdin->dup2(File("stdin"));

    if(stdout)
      stdout->dup2(File("stdout"));

    if(stderr)
      stderr->dup2(File,"stderr");

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
  destruct(p);

  t=read(0x7fffffff);
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

void system(string s)
{
  object p;
  int pid;
  string t;

  p=file::pipe();
  if(!p) error("System() failed.\n");
  p->set_close_on_exec(0);
  if(pid=fork())
  {
    destruct(p);
    /* Nothing will ever be written here, we are just waiting for it
     * to close
     */
    file::read(1);
  }else{
    exec("/bin/sh","-c",s);
    exit(69);
  }
}

void create()
{
  add_constant("system",system);
  add_constant("exec",exec);
  add_constant("spawn",spawn);
  add_constant("popen",popen);
}
