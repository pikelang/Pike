#define error(X) throw( ({ (X), backtrace()[0..sizeof(backtrace())-2] }) )

import Stdio;

// static private inherit File : file;

#if !constant(strerror)
#define strerror(X) ("errno="+X)
#endif

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



class Spawn
{
   object stdin;
   object stdout;
   object stderr;
   array(object) fd;

   int pid;

   private int low_spawn(array(void|object(Stdio.File)) fdp,
			 array(void|object(Stdio.File)) fd_to_close,
			 string cmd, void|string *args, 
			 void|mapping(string:string) env, 
			 string|void cwd)
   {
      object(Stdio.File) pie,pied; /* interprocess communication */
      int pid;

      pie=Stdio.File();
      pied=pie->pipe();

      if(!(pid=fork()))
      {
	 mixed err=catch
	 {
	    if(cwd && !cd(cwd))
	    {
	       throw(({"pike: cannot change cwd to "+cwd+
		       ": "+strerror(errno())+"\n",
		       backtrace()}));
	    }

	    if (sizeof(fdp)>0 && fdp[0]) fdp[0]->dup2(Stdio.File("stdin"));
	    if (sizeof(fdp)>1 && fdp[1]) fdp[1]->dup2(Stdio.File("stdout"));
	    if (sizeof(fdp)>2 && fdp[2]) fdp[2]->dup2(Stdio.File("stderr"));
	    /* dup2 fdd[3..] here FIXME FIXME */
	    foreach (fd_to_close,object f) 
	       if (objectp(f)) { f->close(); destruct(f); }
	    pie->close();
	    destruct(pie);
	   
	    pied->set_close_on_exec(1);

	    if (env) 
	       exece(cmd,args||({}),env);
	    else 
	       exece(cmd,args||({}));

	    throw(({"pike: failed to exece "+cmd+
		    ": "+strerror(errno())+"\n",
		    backtrace()}));
	 };

	 pied->write(encode_value(err));
	 exit(1);
      }

      foreach (fdp,object f) if (objectp(f)) { f->close(); destruct(f); }

      pied->close();
      destruct(pied);

      mixed err=pie->read();
      if (err && err!="") throw(decode_value(err));

      pie->close();
      destruct(pie);

      return pid;
   }

   void create(string cmd,
	       void|array(string) args,
	       void|mapping(string:string) env,
	       string|void cwd,
	       void|array(object(Stdio.File)|void) ownpipes,
	       void|array(object(Stdio.File)|void) fds_to_close)
   {
      if (!ownpipes)
      {
	 stdin=Stdio.File();
	 stdout=Stdio.File();
	 stderr=Stdio.File();
	 fd=({stdin->pipe(),stdout->pipe(),stderr->pipe()});
	 fds_to_close=({stdin,stdout,stderr});
      }
      else
      {
	 fd=ownpipes;
	 if (sizeof(fd)>0) stdin=fd[0]; else stdin=0;
	 if (sizeof(fd)>1) stdout=fd[1]; else stdout=0;
	 if (sizeof(fd)>2) stderr=fd[2]; else stderr=0;
      }
      pid=low_spawn(fd,fds_to_close||({}),cmd,args,env,cwd);
   }

   int kill(int signal) 
   { 
      return predef::kill(pid,signal); 
   }

   void wait()
   {
      while (kill(0)) sleep(0.01);
   }

   // void set_done_callback(function foo,mixed ... args);
   // int result();
   // array rusage();
}
