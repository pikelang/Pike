#pike __REAL_VERSION__

constant create_process = __builtin.create_process;

#if constant(__builtin.TraceProcess)
constant TraceProcess = __builtin.TraceProcess;
#endif

#if defined(__NT__) || defined(__amigaos__) || defined(__OS2__)
constant path_separator = ";";
#else
constant path_separator = ":";
#endif

#if constant(Stdio.__HAVE_SEND_FD__)
protected Stdio.File forkd_pipe;
protected create_process forkd_pid;

//! Encoder for data to be sent to @[Tools.Standalone.forkd].
//!
//! @seealso
//!   @[ForkdDecoder]
class ForkdEncoder(Stdio.File remote_fd)
{
  int fd_counter;
  mixed nameof(mixed what)
  {
    if (objectp(what)) {
      if (what->_fd) {
	remote_fd->send_fd(what->_fd);
	return fd_counter++;
      }
    }
    error("Unsupported argument for RemoteProcess.\n");
  }
}

//! Decoder for data received by @[Tools.Standalone.forkd].
//!
//! @seealso
//!   @[ForkdEncoder]
class ForkdDecoder(array(Stdio.Fd) fds)
{
  object objectof(mixed code)
  {
    return fds[code];
  }
}

// Ensure that there's a live forkd running.
protected int assert_forkd()
{
  if (!forkd_pid || !forkd_pid->kill(0)) {
    forkd_pipe = Stdio.File();
    forkd_pid = spawn_pike(({ "-x", "forkd" }),
			   ([
			     "forkd":0,
			     "uid":getuid(),
			     "gid":getgid(),
			     "noinitgroups":1,
			     "fds":({
			       forkd_pipe->pipe(Stdio.PROP_BIDIRECTIONAL|
						Stdio.PROP_SEND_FD),
			     }),
			   ]));
    if (forkd_pipe->read(1) != "\0") {
      // werror("Failed to start forkd.\n");
      return 0;
    }
  }
  return forkd_pid && forkd_pid->kill(0);
}
#endif

protected int(0..1) forkd_default = 0;

//! Set the default value for the @expr{"forkd"@} modifier
//! to @[Process].
//!
//! @note
//!   The default default value is @expr{0@} (zero).
//!
//! @seealso
//!   @[get_forkd_default()], @[Process()->create()]
void set_forkd_default(int(0..1) mode)
{
  forkd_default = mode;
}

//! Get the default value for the @expr{"forkd"@} modifier
//! to @[Process].
//!
//! @note
//!   The default default value is @expr{0@} (zero).
//!
//! @seealso
//!   @[set_forkd_default()], @[Process()->create()]
int(0..1) get_forkd_default()
{
  return forkd_default;
}

//! Slightly polished version of @[create_process].
//!
//! In addition to the features supported by @[create_process],
//! it also supports:
//!
//! @ul
//!   @item
//!     Callbacks on timeout and process termination.
//!   @item
//!     Using @[Tools.Standalone.forkd] via RPC to
//!     spawn the new process.
//! @endul
//!
//! @seealso
//!   @[create_process], @[Tools.Standalone.forkd]
class Process
{
  //! Based on @[create_process].
  inherit create_process;

  protected function(Process:void) read_cb;
  protected function(Process:void) timeout_cb;

  protected Stdio.File process_fd;

  protected Pike.Backend process_backend;

  protected mixed process_poll;

  //! @param command_args
  //!   Either a command line array, as the command_args
  //!   argument to @[create_process()], or a string that
  //!   will be splitted into a command line array by
  //!   calling @[split_quoted_string()] in an operating
  //!   system dependant mode.
  //! @param modifiers
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
  //!     @member int(0..1) "forkd"
  //!       Use @[Tools.Standalone.forkd] to actually spawn the process.
  //!   @endmapping
  //!
  //! @note
  //!   The default value for the @expr{"forkd"@} modifier may be
  //!   set via @[set_forkd_default()].
  //!
  //! @seealso
  //!   @[create_process], @[create_process()->create()],
  //!   @[split_quoted_string()], @[Tools.Standalone.forkd],
  //!   @[set_forkd_default()], @[get_forkd_default()]
  protected void create( string|array(string) command_args,
			 void|mapping(string:mixed) modifiers )
  {
    if( stringp( command_args ) ) {
      command_args = split_quoted_string( [string]command_args
#ifdef __NT__
				  ,1
#endif /* __NT__ */
				  );
    }

    mapping(string:mixed) new_modifiers = (modifiers || ([])) + ([]);

    if (new_modifiers->keep_signals) {
      // This option is currently not supported with forkd.
      m_delete(new_modifiers, "forkd");
    }

#if constant(Stdio.__HAVE_SEND_FD__)
    // Forkd mode requires send_fd().
    if (undefinedp(new_modifiers->forkd))
      new_modifiers->forkd = forkd_default;
    if (new_modifiers->forkd && assert_forkd()) {
      process_fd = Stdio.File();
      forkd_pipe->
	send_fd(process_fd->pipe(Stdio.PROP_BIDIRECTIONAL|Stdio.PROP_SEND_FD));
      while (forkd_pipe->write("\0") < 0)
	;

      m_delete(new_modifiers, "forkd");
      __callback = m_delete(new_modifiers, "callback");

      if (undefinedp(new_modifiers->uid)) {
	new_modifiers->uid = geteuid();
	if (undefinedp(new_modifiers->gid)) {
	  new_modifiers->gid = getegid();
	}
	if (!new_modifiers->setgroups) {
	  new_modifiers->setgroups = getgroups();
	}
      } else if (new_modifiers->noinitgroups) {
	if (!new_modifiers->setgroups) {
	  new_modifiers->setgroups = getgroups();
	}
      }
      m_delete(new_modifiers, "noinitgroups");

      if (!new_modifiers->env) {
	new_modifiers->env = getenv();
      }

      if (new_modifiers->cwd) {
	new_modifiers->cwd = combine_path(getcwd(), new_modifiers->cwd);
      } else {
	new_modifiers->cwd = getcwd();
      }

      if (!new_modifiers->stdin) new_modifiers->stdin = Stdio.stdin;
      if (!new_modifiers->stdout) new_modifiers->stdout = Stdio.stdout;
      if (!new_modifiers->stderr) new_modifiers->stderr = Stdio.stderr;

      string data = encode_value(({ command_args, new_modifiers }),
				 ForkdEncoder(process_fd));
      data = sprintf("%4c%s", sizeof(data), data);
      // Wait for forkd to acknowledge the process_fd.
      if (process_fd->read(4) != "\0\0\0\0") {
	process_fd->close();
	process_fd = UNDEFINED;
	error("Failed to create spawn request.\n");
      }
      int bytes = process_fd->write(data);
      if (bytes != sizeof(data)) {
	process_fd->close();
	process_fd = UNDEFINED;
	error("Failed to write spawn request (%d != %d).\n",
	      bytes, sizeof(data));
      }
      process_backend = Pike.SmallBackend();
      process_backend->add_file(process_fd);
      process_fd->set_nonblocking(got_data, UNDEFINED, got_close);

      // Wait for the process to fork (or fail).
      while (__status == -1) {
	process_backend(3600.0);
      }

      if (process_fd) {
	process_poll = call_out(do_poll_loop, 0.1);
      }
    } else
#endif
    {
      ::create( [array(string)]command_args, new_modifiers );
    }

    if (modifiers) {
      if(read_cb = modifiers->read_callback)
	call_out(watcher, 0.1);

      if( (timeout_cb = modifiers->timeout_callback) ||
	  modifiers->timeout )
	call_out(killer, modifiers->timeout||15);
    }
  }

#if constant(Stdio.__HAVE_SEND_FD__)
  protected void do_close()
  {
    if (process_fd) {
      process_fd->set_blocking();
      process_fd->close();
    }
    process_fd = UNDEFINED;
    process_backend = UNDEFINED;
    remove_call_out(process_poll);
  }

  protected string recv_buf = "";
  protected void got_data(mixed ignored, string data)
  {
    recv_buf += data;
    while (sizeof(recv_buf) > 3) {
      while (has_prefix(recv_buf, "\0\0\0\0")) recv_buf = recv_buf[4..];
      if (sizeof(recv_buf) < 5) return;
      int len = 0;
      sscanf(recv_buf, "%04c", len);
      if (sizeof(recv_buf) < len + 4) return;
      data = recv_buf[4..len+3];
      recv_buf = recv_buf[len+4..];
      [string tag, mixed packet] = decode_value(data);

      switch(tag) {
      case "ERROR":
	__result = 255;
	__status = 2;
	do_close();
	error(packet);
	break;
      case "PID":
	__pid = packet;
	if (__status == -1) __status = 0;
	if (__callback) __callback(this_object());
	break;
      case "SIGNAL":
	__last_signal = packet;
	break;
      case "START":
	__status = 0;
	if (__callback) __callback(this_object());
	break;
      case "STOP":
	__status = 1;
	if (__callback) __callback(this_object());
	break;
      case "EXIT":
	__result = packet;
	__status = 2;
	do_close();
	if (__callback) __callback(this_object());
	break;
      default:
	__result = 255;
	__status = 2;
	do_close();
	error("Unsupported packet from forkd: %O\n", tag);
	break;
      }
    }
  }

  protected void got_close(mixed ignored)
  {
    do_close();
    if (__status == -1) {
      // Early close.
      __result = 255;
      __status = 2;
    }
  }

  protected void do_poll(float t)
  {
    mixed err = catch {
	process_backend && process_backend(t);
	return;
      };
    // Filter errors about the backend already running.
    if (arrayp(err) && sizeof(err) &&
	stringp(err[0]) && has_prefix(err[0], "Backend already ")) return;
    throw(err);
  }

  protected void do_poll_loop()
  {
    process_poll = UNDEFINED;
    if (!process_backend) return;
    do_poll(0.0);
    process_poll = call_out(do_poll_loop, 0.1);
  }

  int last_signal()
  {
    do_poll(0.0);
    return ::last_signal();
  }

  int(-1..2) status()
  {
    do_poll(0.0);
    return ::status();
  }

  int wait()
  {
    if (process_backend) {
      do_poll(0.0);
      while (__status <= 0) {
	do_poll(3600.0);
      }
      return __result;
    }
    return ::wait();
  }
#endif /* __HAVE_SEND_FD__ */

  protected void destroy() {
    remove_call_out(watcher);
    remove_call_out(killer);
#if constant(Stdio.__HAVE_SEND_FD__)
    remove_call_out(process_poll);
#endif
  }

  protected void watcher() {
    // It was another sigchld, but not one from our process.
    if(status()==0)
      call_out(watcher, 0.1);
    else {
      remove_call_out(killer);
      if(read_cb) read_cb(this);
    }
  }

  protected void killer() {
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
#ifdef __NT__
  if( !has_suffix(lower_case(name), ".exe") )
    name += ".exe";
#endif

  foreach(path, string dir)
  {
    string fname = combine_path(dir, name);
    Stdio.Stat info = file_stat(fname);
    if (info && (info->mode & 0111))
      return fname;
  }
  return 0;
}

protected array(string) runpike;

//! Spawn a new pike process similar to the current.
//!
//! @param argv
//!   Arguments for the new process.
//!
//! @param options
//!   Process creation options. See @[Process.Process] for details. May also
//!   specify "add_predefines", "add_program_path", or "add_include_path" in
//!   order to include these components in command path (module path is 
//!   included by default.)
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
      res+=({"-M" + path});
    if(options && options->add_predefines)
    {
      foreach (master()->predefines; string key; string value)
	if( stringp( value ) )
	  res+=({"-D" + key + "=" + value});
	else if( intp( value ) )
	  res+=({"-D" + key });
    }
    if(options && options->add_program_path)
    {
      foreach (master()->pike_program_path;; string value)
        res+=({"-P" + value});
    }
    if(options && options->add_include_path)
    {
      foreach (master()->pike_include_path;; string value)
        res+=({"-I" + value});
    }

    if (sizeof(res) && !has_value(res[0],"/")
#ifdef __NT__
	&& !has_value(res[0], "\\")
#endif /* __NT__ */
	)
      res[0] = search_path(res[0]);
    runpike = res;
  }
  return Process(runpike + argv, options);
}

//! Easy and lazy way of using @[Process.Process] that runs a process 
//! and returns a mapping with the output and exit code without
//! having to make sure you read nonblocking yourself.
//!
//! @param args
//!   Either a command line array, as the command_args
//!   argument to @[create_process()], or a string that
//!   will be splitted into a command line array by
//!   calling @[split_quoted_string()] in an operating
//!   system dependant mode.
//! @param modifiers
//!   It takes all the modifiers @[Process.Process] accepts, with 
//!   the exception of stdout and stderr. Since the point of this 
//!   function is to handle those you can not supply your own.
//!
//!   If @expr{modifiers->stdin@} is set to a string it will automaticly be
//!   converted to a pipe that is fed to stdin of the started process.
//!
//! @seealso
//!   @[Process.Process] @[create_process]
//!
//! @returns
//!   @mapping
//!     @member string "stdout"
//!       Everything the process wrote on stdout.
//!     @member string "stderr"
//!       Everything the process wrote on stderr.
//!     @member int "exitcode"
//!       The process' exitcode.
//!   @endmapping
//!
//! @note 
//!   As the entire output of stderr and stdout is stored in the 
//!   returned mapping it could potentially grow until memory runs out. 
//!   It is therefor adviceable to set up rlimits if the output has a
//!   potential to be very large.
//!
//! @example
//!   Process.run( ({ "ls", "-l" }) );
//!   Process.run( ({ "ls", "-l" }), ([ "cwd":"/etc" ]) );
//!   Process.run( "ls -l" );
//!   Process.run( "awk -F: '{print $2}'", ([ "stdin":"foo:2\nbar:17\n" ]) );
mapping run(string|array(string) cmd, void|mapping modifiers)
{
  string gotstdout="", gotstderr="", stdin_str;
  int exitcode;

  if(!modifiers)
    modifiers = ([]);

  if(modifiers->stdout || modifiers->stderr)
    throw( ({ "Can not redirect stdout or stderr in Process.run, "
              "please use Process.Process instead.", backtrace() }) );

  Stdio.File mystdout = Stdio.File(); 
  Stdio.File mystderr = Stdio.File();
  Stdio.File mystdin;

  object p;
  if(stringp(modifiers->stdin))
  {
    mystdin = Stdio.File();
    stdin_str = modifiers->stdin;
    p = Process(cmd, modifiers + ([ 
                  "stdout":mystdout->pipe(),
                  "stderr":mystderr->pipe(),
                  "stdin":mystdin->pipe(Stdio.PROP_IPC|Stdio.PROP_REVERSE)
                ]));
  }
  else
    p = Process(cmd, modifiers + ([ 
                  "stdout":mystdout->pipe(),
                  "stderr":mystderr->pipe(),
                ]));

#if 0 //constant(Thread.Thread)
  // This is disabled by default since the callback alternative is
  // much more lightweight - creating threads isn't cheap.
  array threads = ({
      thread_create( lambda() { gotstdout = mystdout->read(); } ),
      thread_create( lambda() { gotstderr = mystderr->read(); } )
    });

  if (mystdin) {
    threads += ({
      thread_create(lambda(Stdio.File f) { f->write(stdin_str); f->close(); }, mystdin )
    });
    mystdin = 0;
  }

  exitcode = p->wait();
  threads->wait();
#else
  Pike.SmallBackend backend = Pike.SmallBackend();

  mystdout->set_backend (backend);
  mystderr->set_backend (backend);

  mystdout->set_read_callback( lambda( mixed i, string data) { 
                                 gotstdout += data; 
                               } );
  mystderr->set_read_callback( lambda( mixed i, string data) {
                                 gotstderr += data;
                               } );
  mystdout->set_close_callback( lambda () {
				  mystdout->set_read_callback(0);
				  catch { mystdout->close(); };
				  mystdout = 0;
				});
  mystderr->set_close_callback( lambda () {
				  mystderr->set_read_callback(0);
				  catch { mystderr->close(); };
				  mystderr = 0;
				});

  if (mystdin) {
    Shuffler.Shuffler sfr = Shuffler.Shuffler();
    sfr->set_backend (backend);
    Shuffler.Shuffle sf = sfr->shuffle( mystdin );
    sf->add_source(stdin_str);
    sf->set_done_callback (lambda () {
			     catch { mystdin->close(); };
			     mystdin = 0;
			   });
    sf->start();
  }

  while( mystdout || mystderr || mystdin )
    backend( 1.0 );

  exitcode = p->wait();
#endif

  return ([ "stdout"  : gotstdout,
            "stderr"  : gotstderr,
            "exitcode": exitcode   ]);
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

protected string search_path_raw;
protected array(string) search_path_entries=0;

//! Search for the path to an executable.
//!
//! @param command
//!   Executable to search for.
//!
//! Searches for @[command] in the directories listed in the
//! environment variable @tt{$PATH@}.
//!
//! @returns
//!   Returns the path to @[command] if found, and
//!   @expr{0@} (zero) on failure.
//!
//! @note
//!   This function is NOT thread safe if the environment
//!   variable @tt{$PATH@} is being changed concurrently.
//!
//! @note
//!   In Pike 7.8.752 and earlier the environment
//!   variable @tt{$PATH@} was only read once.
string search_path(string command)
{
   if (command=="" || command[0]=='/') return command;

   string path = getenv("PATH")||"";
   if ((path != search_path_raw) || !search_path_entries)
   {
      string raw_path = path;
#ifdef __NT__
      path = replace(path, "\\", "/");
#endif
      array(string) e = path/path_separator - ({""});

      multiset(string) filter=(<>);
      array(string) entries=({});
      foreach (e,string s)
      {
	 string t;
	 if (s[0]=='~')  // some shells allow ~-expansion in PATH
	 {
	    if (s[0..1]=="~/" && (t=System.get_home()))
	       s=t+s[1..];
	    else
	    {
	       // expand user?
	    }
#ifdef __NT__
	 } else if (sizeof(s) && (s[0] == '"') && s[-1] == '"') {
	   // Windows doesn't seem to strip quotation marks from PATH
	   // components that contain them so we need to do that here,
	   // otherwise we'll end up with a bogus path to stat on.
	   s = s[1..<1];
#endif /* __NT__ */
	 }
	 if (!filter[s] /* && directory exist */ )
	 {
	    entries += ({s});
	    filter[s]=1;
	 }
      }
      // FIXME: This is NOT thread safe!
      search_path_entries = entries;
      search_path_raw = raw_path;
   }
   return locate_binary(search_path_entries, command);
}

//!
string sh_quote(string s)
{
  return replace(s,
	({"\\", "'", "\"", " "}),
	({"\\\\", "\\'", "\\\"","\\ "}));
}

array(string) split_quoted_string(string s, int(0..1)|void nt_mode)
//! Splits the given string into a list of arguments, according to
//! common (i.e. @expr{/bin/sh@}-based) command line quoting rules:
//!
//! @ul
//! @item
//!   Sequences of whitespace, i.e. space, tab, @expr{\n@} or
//!   @expr{\r@}, are treated as argument separators by default.
//!
//! @item
//!   Single or double quotes (@expr{'@} or @expr{"@}) can be used
//!   around an argument to avoid whitespace splitting inside it. If
//!   such quoted strings are used next to each other then they get
//!   concatenated to one argument; e.g. @expr{a"b"'c'@} becomes a
//!   single argument @expr{abc@}.
//!
//! @item
//!   Backslash (@expr{\@}) can be used in front of one of the space
//!   or quote characters mentioned above to treat them literally.
//!   E.g. @expr{x\ y@} is a single argument with a space in the
//!   middle, and @expr{x\"y@} is a single argument with a double
//!   quote in the middle.
//!
//! @item
//!   A backslash can also be used to quote itself; i.e. @expr{\\@}
//!   becomes @expr{\@}.
//!
//! @item
//!   Backslashes in front of other characters are removed by default.
//!   However, if the optional @[nt_mode] flag is set then they are
//!   retained as-is, to work better with Windows style paths.
//!
//! @item
//!   Backslashes are treated literally inside quoted strings, with
//!   the exception that @expr{\"@} is treated as a literal @expr{"@}
//!   inside a @expr{"@}-quoted string. It's therefore possible to
//!   include a literal @expr{"@} in a @expr{"@}-quoted string, as
//!   opposed to @expr{'@}-quoted strings which cannot contain a
//!   @expr{'@}.
//! @endul
{
  // Remove initial white-space.
  sscanf(s,"%*[ \t\n\r]%s",s);

  // Prefix interesting characters with NUL,
  // and split on NUL.
  s=replace(s,
	    ({"\"",  "'",  "\\",  " ",  "\t",  "\n",   "\r",   "\0"}),
	    ({"\0\"","\0'","\0\\","\0 ","\0\t","\0\n", "\0\r", "\0\0"}));
  array(string) x=s/"\0";
  array(string) ret = ({});
  string last = x[0];
  int always_keep_last;

piece_loop:
  for(int e=1;e<sizeof(x);e++)
  {
    string piece = x[e];
    if (piece == "") {
      // Escaped NUL. There should always be another element in x.
      last += "\0" + x[++e];
      continue;
    }
    switch(piece[0])
    {
      case '"':
      always_keep_last = 1;
      last+=piece[1..];
      while (1) {
	if (++e == sizeof (x))
	  break piece_loop;
	if (has_prefix (piece = x[e], "\""))
	  break;
	if (piece == "") {	// Escaped NUL.
	  last += "\0" + x[++e];
	}
	else {
	  if(piece == "\\" && sizeof (x) > e + 1 && has_prefix (x[e+1], "\""))
	    piece = x[++e];
	  last+=piece;
	}
      }
      last+=piece[1..];
      break;

      case '\'':
      always_keep_last = 1;
      last+=piece[1..];
      while (1) {
	if (++e == sizeof (x))
	  break piece_loop;
	if (has_prefix (piece = x[e], "'"))
	  break;
	if (piece == "") {	// Escaped NUL.
	  last += "\0" + x[++e];
	}
	else
	  last+=piece;
      }
      last+=piece[1..];
      break;
      
      case '\\':
      if(sizeof(piece)>1)
      {
	if (nt_mode) {
	  // On NT we only escape special characters with \;
	  // other \'s we keep verbatim.
	  last+=piece;
	} else {
	  last+=piece[1..];
	}
      }else if (sizeof (x) > e + 1) {
	// Escaped special character.
	last+=x[++e];
      }
      break;
      
      case ' ':
      case '\t':
      case '\n':
      case '\r':
	while(sizeof(piece)==1)
	{
	  if(e+1 < sizeof(x))
	  {
	    if((<' ','\t','\n', '\r'>) [x[e+1][0]])
	      piece = x[++e];
	    else
	      break;
	  }else{
	    break piece_loop;
	  }
	}
	if (sizeof (last) || always_keep_last)
	  ret += ({last});
	last = piece[1..];
      break;
    }
  }
  if (sizeof (last) || always_keep_last)
    ret += ({last});
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
  return Process(({ "cmd", "/c", command }),data);
#elif defined(__amigaos__)
  return Process(split_quoted_string(command),data);
#else /* !__NT__||__amigaos__ */
  return Process(({ "/bin/sh", "-c", command }),data);
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

variant string popen(string s) {
   return fpopen(s)->read();
}

variant Stdio.FILE popen(string s, string mode) {
  return fpopen(s,mode);
}

protected Stdio.FILE fpopen(string s, string|void mode)
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

#if constant(fork) || constant(System.daemon)
private int low_daemon(int nochdir, int noclose)
{
#if constant(System.daemon)
    return System.daemon(nochdir, noclose);
#else
    if (fork())
        exit(0);

#if constant(System.setsid)
        System.setsid();
#endif /* System.setsid */

    if (!nochdir)
        cd("/");

    Stdio.File fd;
    if (!noclose && (fd = Stdio.File("/dev/null", "rw")))
    {
        fd->dup2(Stdio.stdin);
        fd->dup2(Stdio.stdout);
        fd->dup2(Stdio.stderr);
        if (fd->query_fd() > 2)
            fd->close();
    }
    return 0;
#endif /* !System.daemon */
}

void daemon(int nochdir, int noclose,
            void|mapping(string:string|Stdio.File) modifiers)
//! A function to run current program in the background.
//!
//! @param nochdir
//!   If 0 the process will continue to run in / or the directory
//!   dictadet by modifiers.
//! @param noclose
//!  If this is not 0 the process will keep current file descriptors
//!  open.
//! @param modifiers
//!   Optional extra arguments. The parameters passed in this mapping
//!   will override the arguments nochdir and noclose.
//!   @mapping
//!     @member string "cwd"
//!       Change current working directory to this directory.
//!     @member string|Stdio.File "stdin"
//!       If this is a string this will be interpreted as a filename
//!       pointing out a file to be used as stdandard input to the process.
//!       If this is a Stdio.File object the process will use this as
//!       standard input.
//!     @member string|Stdio.File "stdout"
//!       If this is a string this will be interpreted as a filename
//!       pointing out a file to be used as stdandard output to the process.
//!       If this is a Stdio.File object the process will use this as
//!       standard output.
//!     @member string|Stdio.File "stderr"
//!       If this is a string this will be interpreted as a filename
//!       pointing out a file to be used as stdandard error to the process.
//!       If this is a Stdio.File object the process will use this as
//!       standard error.
//!   @endmapping
//!
//! @seealso
//!   @[System.daemon]
//!
//! @note
//!   This function only works on UNIX-like operating systems.
//!
//! @example
//!    /* close all fd:s and cd to '/' */
//!   Process.daemon(0, 0);
//!
//!   /* Do not change working directory. Write stdout to a file called
//!      access.log and stderr to error.log. */
//!   Process.daemon(1, 0, ([ "stdout": "access.log", "stderr": "error.log" ]) );
{
    array(Stdio.File) opened = ({});
    Stdio.File getfd(string|object f)
    {
        if (stringp(f))
        {
            Stdio.File ret = Stdio.File(f, "crw");
            opened += ({ ret });
            return ret;
        }
        else if (objectp(f))
            return f;
	else
	  return 0;
    };

    if (low_daemon(nochdir, noclose) == -1)
      error("Failed to daemonize: " + strerror(errno())+"\n");
    if (!modifiers)
        return;

    if (modifiers["cwd"])
        cd(modifiers["cwd"]);

    if (modifiers["stdin"])
        getfd(modifiers["stdin"])->dup2(Stdio.stdin);

    if (modifiers["stdout"])
        getfd(modifiers["stdout"])->dup2(Stdio.stdout);

    if (modifiers["stderr"])
        getfd(modifiers["stderr"])->dup2(Stdio.stderr);

    opened->close();
}
#endif
