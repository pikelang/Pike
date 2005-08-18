#pike __REAL_VERSION__

constant create_process = __builtin.create_process;

#if constant(__builtin.TraceProcess)
constant TraceProcess = __builtin.TraceProcess;
#endif

//! Slightly polished version of @[create_process].
//! @seealso
//!   @[create_process]
class Process
{
  inherit create_process;
  static function(Process:void) read_cb;
  static function(Process:void) timeout_cb;

  //! @param args
  //!   Either a command line array, as the command_args
  //!   argument to @[create_process()], or a string that
  //!   will be splitted into a command line array by
  //!   calling @[split_quoted_string()] in an operating
  //!   system dependant mode.
  //! @param m
  //!   In addition to the modifiers that @[create_process] accepts,
  //!   this object also accepts
  //!   @mapping
  //!     @member function(Process:void) "read_callback"
  //!       This callback is called when there is data to be read
  //!       from the process.
  //!     @member function(Process:void) "timeout_callback"
  //!       This callback is called if the process times out.
  //!     @member int "timeout"
  //!       The time it takes for the process to time out. Default is
  //!       15 seconds.
  //!   @endmapping
  //! @seealso
  //!   @[create_process], @[split_quoted_string]
  static void create( string|array(string) args, void|mapping(string:mixed) m )
  {
    if( stringp( args ) ) {
      args = split_quoted_string( [string]args
#ifdef __NT__
				  ,1
#endif /* __NT__ */
				  );
    }
    if( !m )
      ::create( [array(string)]args );
    else {
      ::create( [array(string)]args, [mapping(string:mixed)]m );

      if(read_cb=m->read_callback)
	call_out(watcher, 0.1);

      if( (timeout_cb=m->timeout_callback) || m->timeout )
	call_out(killer, m->timeout||15);
    }
  }

  static void destroy() {
    remove_call_out(watcher);
    remove_call_out(killer);
  }

  static void watcher() {
    // It was another sigchld, but not one from our process.
    if(::status()==0)
      call_out(watcher, 0.1);
    else {
      remove_call_out(killer);
      if(read_cb) read_cb(this);
    }
  }

  static void killer() {
    remove_call_out(watcher);
#if constant(kill)
    ::kill(signum("SIGKILL"));
#endif
    if(timeout_cb) timeout_cb(this);
  }
}

// FIXME: Should probably be located elsewhere.
string locate_binary(array(string) path, string name)
{
  string dir;
  Stdio.Stat info;
  foreach(path, dir)
  {
    string fname = dir + "/" + name;
    if ((info = file_stat(fname))
	&& (info[0] & 0111))
      return fname;
  }
  return 0;
}

static array(string) runpike;

//! Spawn a new pike process similar to the current.
//!
//! @param argv
//!   Arguments for the new process.
//!
//! @param options
//!   Process creation options. See @[Process.Process] for details.
//!
//! @seealso
//!   @[Process.Process]
Process spawn_pike(array(string) argv, void|mapping(string:mixed) options)
{
  if (!runpike) {
    array(string) res = ({
      master()->_pike_file_name,
    });
    if (master()->_master_file_name)
      res+=({"-m"+master()->_master_file_name});
    foreach (master()->pike_module_path;;string path)
      res+=({"-M"+path});

    // FIXME: Defines? pike_program_path?

    if (sizeof(res) && !has_value(res[0],"/")
#ifdef __NT__
	&& !has_value(res[0], "\\")
#endif /* __NT__ */
	)
      res[0] = locate_binary(getenv("PATH")/
#if defined(__NT__) || defined(__amigaos__)
			     ";"
#else
			     ":"
#endif
			     ,res[0]);
    runpike = res;
  }
  return Process(runpike + argv, options);
}

#if constant(exece)
//!
int exec(string file,string ... foo)
{
  if (sizeof(file)) {
    string path;

    if(has_value(file,"/"))
      return exece(combine_path(getcwd(),file),foo,
		   [mapping(string:string)]getenv());

    path=[string]getenv("PATH");

    foreach(path ? path/":" : ({}) , path)
      if(file_stat(path=combine_path(path,file)))
	return exece(path, foo, [mapping(string:string)]getenv());
  }
  return 69;
}
#endif

static array(string) search_path_entries=0;

//!
string search_path(string command)
{
   if (command=="" || command[0]=='/') return command;

   if (!search_path_entries) 
   {
      array(string) e=(getenv("PATH")||"")/":"-({""});

      multiset(string) filter=(<>);
      search_path_entries=({});
      foreach (e,string s)
      {
	 string t;
	 if (s[0]=='~')  // some shells allow ~-expansion in PATH
	 {
	    if (s[0..1]=="~/" && (t=[string]getenv("HOME")))
	       s=t+s[1..];
	    else
	    {
	       // expand user?
	    }
	 }
	 if (!filter[s] /* && directory exist */ ) 
	 {
	    search_path_entries+=({s}); 
	    filter[s]=1;
	 }
      }
   }
   foreach (search_path_entries, string path)
   {
      string p=combine_path(path,command);
      Stdio.Stat s=file_stat(p);
      if (s && s->mode&0111) return p;
   }
   return 0;
}

//!
string sh_quote(string s)
{
  return replace(s,
	({"\\", "'", "\"", " "}),
	({"\\\\", "\\'", "\\\"","\\ "}));
}

//!
array(string) split_quoted_string(string s, int(0..1)|void nt_mode)
{
  // Remove initial white-space.
  sscanf(s,"%*[ \n\t]%s",s);

  // Prefix interresting characters with NUL,
  // and split on NUL.
  s=replace(s,
	    ({"\"",  "'",  "\\",  " ",  "\t",  "\n", "\0"}),
	    ({"\0\"","\0'","\0\\","\0 ","\0\t","\0\n", "\0\0"}));
  array(string) x=s/"\0";
  array(string) ret=({x[0]});

  for(int e=1;e<sizeof(x);e++)
  {
    if (!sizeof(x[e])) {
      // Escaped NUL.
      ret[-1] += "\0";
      e++;
      continue;
    }
    switch(x[e][0])
    {
      case '"':
      ret[-1]+=x[e][1..];
      while(x[++e][0]!='"')
      {
	if(sizeof(x[e])==1 && x[e][0]=='\\' && x[e+1][0]=='"') e++;
	ret[-1]+=x[e];
      }
      ret[-1]+=x[e][1..];
      break;

      case '\'':
      ret[-1]+=x[e][1..];
      while(x[++e][0]!='\'') ret[-1]+=x[e];
      ret[-1]+=x[e][1..];
      break;
      
      case '\\':
      if(sizeof(x[e])>1)
      {
	if (nt_mode) {
	  // On NT we only escape special characters with \;
	  // other \'s we keep verbatim.
	  ret[-1]+=x[e];
	} else {
	  ret[-1]+=x[e][1..];
	}
      }else{
	// Escaped special character.
	ret[-1]+=x[++e];
      }
      break;
      
      case ' ':
      case '\t':
      case '\n':
	while(sizeof(x[e])==1)
	{
	  if(e+1 < sizeof(x))
	  {
	    if((<' ','\t','\n'>) [x[e+1][0]])
	      e++;
	    else
	      break;
	  }else{
	    return ret;
	  }
	}
	ret+=({x[e][1..]});
      break;

      default:
	ret[-1]+="\0"+x[e];
	break;
    }
  }
  return ret;
}

Process spawn(string command, void|Stdio.Stream stdin,
	      void|Stdio.Stream stdout, void|Stdio.Stream stderr,
	      // These aren't used. Seems to be part of something unfinished. /mast
	      //function|void cleanup, mixed ... args
	     )
//! Spawns a process that executes @[command] as a command shell
//! statement ("@expr{/bin/sh -c @[command]@}" for Unix, "@expr{cmd /c
//! @[command]@}" for Windows).
//!
//! @param stdin
//! @param stdout
//! @param stderr
//!   Stream objects to use as standard input, standard output and
//!   standard error, respectively, for the created process. The
//!   corresponding streams for this process are used for those that
//!   are left out.
//!
//! @returns
//!   Returns a @[Process.Process] object for the created process.
//!
//! @seealso
//!   @[system], @[popen]
{
  mapping(string:mixed) data=(["env":getenv()]);
  if(stdin) data->stdin=stdin;
  if(stdout) data->stdout=stdout;
  if(stderr) data->stderr=stderr;
#if defined(__NT__)
  // if the command string command is not quoted, add double quotes to
  // make sure it is not modified by create_process
  if (sizeof(command) <= 1 ||
      command[0] != '\"' || command[sizeof(command)-1] != '\"')
    command = "\"" + command + "\"";
  return create_process(({ "cmd", "/c", command }),data);
#elif defined(__amigaos__)
  return create_process(split_quoted_string(command),data);
#else /* !__NT__||__amigaos__ */
  return create_process(({ "/bin/sh", "-c", command }),data);
#endif /* __NT__||__amigaos__ */

}

//! @decl string popen(string command)
//! Executes @[command] as a shell statement ("@expr{/bin/sh -c
//! @[command]@}" for Unix, "@expr{cmd /c @[command]@}" for Windows),
//! waits until it has finished and returns the result as a string.
//!
//! @seealso
//!   @[system], @[spawn]

//! @decl Stdio.FILE popen(string command, string mode)
//! Open a "process" for reading or writing.  The @[command] is executed
//! as a shell statement ("@expr{/bin/sh -c @[command]@}" for Unix,
//! "@expr{cmd /c @[command]@}" for Windows). The parameter @[mode]
//! should be one of the following letters:
//! @string
//!   @value "r"
//!   Open for reading.  Data written by the process to stdout
//!   is available for read.
//!   @value "w"
//!   Open for writing.  Data written to the file is available
//!   to the process on stdin.
//! @endstring
//!
//! @seealso
//!   @[system], @[spawn]

Stdio.FILE|string popen(string s, string|void mode) {
  if(mode)
    return fpopen(s,mode);
  else
    return fpopen(s)->read();
}

static Stdio.FILE fpopen(string s, string|void mode)
{
  Stdio.FILE f = Stdio.FILE();
  if (!f) error("Popen failed. (couldn't create file)\n");

  Stdio.File p = f->pipe();
  if(!p) error("Popen failed. (couldn't create pipe)\n");

  if (mode == "w")
    spawn(s, p, 0, 0, /*destruct, f*/);
  else
    spawn(s, 0, p, 0, /*destruct, f*/);
  p->close();
  destruct(p);

  return f;
}

int system(string command, void|Stdio.Stream stdin,
	   void|Stdio.Stream stdout, void|Stdio.Stream stderr)
//! Executes @[command] as a shell statement ("@expr{/bin/sh -c
//! @[command]@}" for Unix, "@expr{cmd /c @[command]@}" for Windows),
//! waits until it has finished and returns its return value.
//!
//! @param stdin
//! @param stdout
//! @param stderr
//!   Stream objects to use as standard input, standard output and
//!   standard error, respectively, for the created process. The
//!   corresponding streams for this process are used for those that
//!   are left out.
//!
//! @seealso
//!   @[spawn], @[popen]
{
  return spawn(command, stdin, stdout, stderr)->wait();
}

#ifndef __NT__
#if constant(fork)
constant fork = predef::fork;
#endif

#if constant(exece)
constant exece = predef::exece;
#endif

#if constant(fork) && constant(exece)

//!
class Spawn
{
   object stdin;
   object stdout;
   object stderr;
   array(object) fd;

   object pid;

   private object low_spawn(array(Stdio.File) fdp,
			    array(Stdio.File) fd_to_close,
			    string cmd, void|array(string) args, 
			    void|mapping(string:string) env, 
			    string|void cwd)
   {
      Stdio.File pie, pied; // interprocess communication
      object pid;

      pie=Stdio.File();
      pied=pie->pipe();

      if(!(pid=fork()))
      {
	 mixed err=catch
	 {
	    if(cwd && !cd(cwd))
	    {
	       error( "pike: cannot change cwd to "+cwd+
		      ": "+strerror(errno())+"\n" );
	    }

	    if (sizeof(fdp)>0 && fdp[0]) fdp[0]->dup2(Stdio.File("stdin"));
	    if (sizeof(fdp)>1 && fdp[1]) fdp[1]->dup2(Stdio.File("stdout"));
	    if (sizeof(fdp)>2 && fdp[2]) fdp[2]->dup2(Stdio.File("stderr"));
	    /* dup2 fdd[3..] here FIXME FIXME */
	    foreach (fd_to_close, Stdio.File f)
	       if (objectp(f)) { f->close(); destruct(f); }
	    pie->close();
	    destruct(pie);
	   
	    pied->set_close_on_exec(1);

	    if (env) 
	       exece(cmd,args||({}),env);
	    else 
	       exece(cmd,args||({}));

	    error( "pike: failed to exece "+cmd+
		   ": "+strerror(errno())+"\n" );
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

  //!
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

#if constant(kill)
   //!
   int kill(int signal)
   { 
      return predef::kill(pid,signal); 
   }
#endif

   //!
   int wait()
   {
     return pid->wait();
   }

   // void set_done_callback(function foo,mixed ... args);
   // int result();
   // array rusage();
}
#endif
#endif
