#if !constant(strerror)
#define strerror(X) X
#endif

#if 0
#define PATH_MAP_MSG(X...) werror (X)
#else
#define PATH_MAP_MSG(X...)
#endif

#ifdef __NT__
void exece(string cmd, array(string) args)
{
  exit(Process.create_process( ({ cmd }) + args )->wait());
}
#endif

#if !constant(zero)
typedef int(0..0)	zero;
#endif

#if __VERSION__ < 8.0
#define protected static
#endif

// Constants taken from WinNT.h
constant status_codes = ([
  0x00000080:"Abandoned Wait 0",
  0x000000C0:"User APC",
  0x00000102:"Timeout",
  0x00000103:"Pending",
  0x00010001:"Exception Handled (DBG)",
  0x00010002:"Continue (DBG)",
  0x40000005:"Segment Notification",
  0x40010003:"Terminate Thread (DBG)",
  0x40010004:"Terminate Process (DBG)",
  0x40010005:"Control C (DBG)",
  0x40010008:"Control Break (DBG)",
  0x80000001:"Guard Page Violation",
  0x80000002:"Datatype Misalignment",
  0x80000003:"Breakpoint",
  0x80000004:"Single Step",
  0x80010001:"Exception Not Handled (DBG)",
  0xC0000005:"Access Violation",
  0xC0000006:"In Page Error",
  0xC0000008:"Invalid Handle",
  0xC0000017:"No Memory",
  0xC000001D:"Illegal Instruction",
  0xC0000025:"Noncontinuable Exception",
  0xC0000026:"Invalid Disposition",
  0xC000008C:"Array Bounds Exceeded",
  0xC000008D:"Denormal Operand (Float)",
  0xC000008E:"Divide By Zero (Float)",
  0xC000008F:"Inexact Result (Float)",
  0xC0000090:"Invalid Operation (Float)",
  0xC0000091:"Overflow (Float)",
  0xC0000092:"Stack Check (Float)",
  0xC0000093:"Underflow (Float)",
  0xC0000094:"Divide By Zero (Integer)",
  0xC0000095:"Overflow (Integer)",
  0xC0000096:"Priviledged Instruction",
  0xC00000FD:"Stack Overflow",
  0xC000013A:"Control C Exit",
  0xC00002B4:"Multiple Faults (Float)",
  0xC00002B5:"Multiple Traps (Float)",
  0xC00002C9:"Reg NAT Consumption",
]);
string explain(int code)
{
  string res="";
  switch(code & 0xc0000000) {
  case 0x00000000:
    res = "Success:";
    break;
  case 0x40000000:
    res = "Informational:";
    break;
  case 0x80000000:
    res = "Warning:";
    break;
  case 0xc0000000:
    res = "Error:";
    break;
  }
  if (status_codes[code]) {
    res += status_codes[code];
  } else {
    res += "Unknown";
  }
  return res;
}

string follow_symlinks(string s)
{
  mixed st;
  string x=s;
  while ((st = file_stat(s, 1)) && (st[1] == -3)) {
    string new_s = readlink(s);
    if (new_s == s) {
      werror(sprintf("%O is a symlink to itself!\n", s));
      exit(1);
    }
    s = combine_path(getcwd(),dirname(s),new_s);
  }
//  werror("Follow symlink %O -> %O\n",x,s);
  return s;
}

mapping(string:string) wsl_path_cache = ([]);

string wsl_fixpath(string s)
{
  if (wsl_path_cache[s]) return wsl_path_cache[s];

  object(Stdio.File) f = Stdio.File();
  object(Stdio.File) o = f->pipe(Stdio.PROP_IPC);
  object proc = Process.create_process(({"wslpath", "-w", s}),
                                       (["stdout":o]));
  destruct(o);
  string path = ((f->read() || "")/"\n")[0];
  destruct(f);
  if (proc->wait() || !sizeof(path))
    error("wslpath failed for %O.\n", s);
  if (has_suffix(path, "\r")) path = path[..<1];
  return wsl_path_cache[s] = path;
}

// Allow several mappings of unix paths to NT drives using
// NTMOUNT/NTDRIVE, NTMOUNT1/NTDRIVE1, NTMOUNT2/NTDRIVE2, etc. The
// replace code is naive, so you better have the longest NTMOUNTn
// prefix first.

// Note: Windows handles "/" as path separators but we take care to
// replace them with "\" anyway since a path starting with "/" can be
// mistaken as a command option by some Windows programs.

array(array(string)) pathmap;

array(array(string)) get_pathmap()
{
  if (!pathmap) {
    pathmap = ({});
    string mnt;
    for (int i = 0; (mnt = getenv("NTMOUNT" + (i || ""))) || i < 2; i++)
      if (mnt && mnt != "") {
	string drv = getenv ("NTDRIVE" + (i || ""));
	if (drv && drv != "") {
	  if (has_suffix (mnt, "/")) mnt = mnt[..sizeof (mnt) - 2];
	  drv = replace (drv, "/", "\\");
	  if (has_suffix (drv, "\\")) drv = drv[..sizeof (drv) - 2];
	  PATH_MAP_MSG ("Path map: %O -> %O\n", mnt, drv);
	  pathmap += ({({mnt, drv})});
	}
	else
	  werror ("Got NTMOUNT" + (i || "") + "=%O "
		  "without corresponding NTDRIVE" + (i || "") + ".\n",
		  mnt);
      }
  }
  return pathmap;
}

string fixpath(string s)
{
  PATH_MAP_MSG ("fixpath: %O -> ", s);
  s=follow_symlinks(s);
  PATH_MAP_MSG ("%O -> ", s);
  if (lower_case(getenv("REMOTE_METHOD") || "") == "wsl" &&
      has_prefix(s, "/"))
    return wsl_fixpath(s);
  foreach (get_pathmap(), [string mnt, string drv]) {
    if (mnt == "/") {
      // Special case for whole filesystem.
      if (has_prefix(s, mnt)) {
	s = drv + replace(s, "/", "\\");
      }
      break;
    }
    if (s == mnt || has_prefix (s, mnt) && s[sizeof (mnt)] == '/') {
      s = drv + replace (s[sizeof (mnt)..], "/", "\\");
      break;
    }
  }
  PATH_MAP_MSG ("%O\n", s);
  return s;
}

void fix_paths_in_arglist (array(string) args)
{
  if (lower_case(getenv("REMOTE_METHOD") || "") == "wsl") {
    // Keep the executable as a Linux path for WSL interop.
    foreach (args; int e; string arg)
      if (e && has_prefix(arg, "/") && search(arg[1..], "/") >= 0 &&
          (file_stat(arg) || file_stat(dirname(arg))))
        args[e] = fixpath(arg);
    return;
  }
  get_pathmap();
  foreach (args; int e; string arg)
    foreach (pathmap, [string mnt, string drv]) {
      int i = search (arg, mnt);
      if (i >= 0 && (sizeof (arg) == i + sizeof (mnt) ||
		     arg[i + sizeof (mnt)] == '/')) {
	args[e] = arg[..i - 1] + drv +
	  replace (arg[i + sizeof (mnt)..], "/", "\\");
	PATH_MAP_MSG ("Fixed path in arg: %O -> %O\n", arg, args[e]);
	break;
      }
    }
}

string fixabspath(string s)
{
  return replace(s,"/","\\");
}

string opt_path(string p1, string p2)
{
  return  ( ( ((p1||"") + ";" + (p2||"")) / ";" ) - ({""}) ) * ";";
}

protected void watchdog(int|void sig)
{
  // Reinstate the alarm.
  alarm(1);
}

// ssh passes a single command string to the remote Windows shell.
// Preserve argument boundaries when the shell starts the Windows program.
string quote_ssh_arg(string arg)
{
  if (!sizeof(arg)) return "\"\"";
  if (search(arg, " ") < 0 && search(arg, "\t") < 0 &&
      search(arg, "\"") < 0 && search(arg, "&") < 0 &&
      search(arg, "|") < 0 && search(arg, "<") < 0 &&
      search(arg, ">") < 0 && search(arg, "^") < 0)
    return arg;

  string quoted = "\"";
  int backslashes = 0;
  foreach(arg; int i; int c) {
    if (c == '\\') {
      backslashes++;
    } else {
      quoted += "\\" * (backslashes * (c == '"' ? 2 : 1));
      if (c == '"') quoted += "\\";
      quoted += sprintf("%c", c);
      backslashes = 0;
    }
  }
  return quoted + "\\" * (backslashes * 2) + "\"";
}

// A short blocking read means the reply was truncated or the socket failed.
// In particular, a missing exit status must never be treated as success.
protected string read_sprsh_reply(object socket, int length, string part)
{
  string data = socket->read(length);
  if (!data || sizeof(data) != length) {
    int err = socket->errno();
    werror("sprsh: Incomplete %s (%d of %d bytes): %s.\n",
           part, data ? sizeof(data) : 0, length,
           err ? strerror(err) : "connection closed");
    exit(1);
  }
  return data;
}

int silent_do_cmd(array(string) cmd, mixed|void filter, int|void silent,
		  void|int no_fixpath)
{
//  werror("%O\n",cmd);

  // It seems __read_nocancel() in libpthread on Linux sometimes can hang
  // if the socket enters FIN_WAIT2. Getting a signal releases the syscall
  // and the process comes alive again.
  signal(signum("SIGALRM"), watchdog);
  alarm(1);

  if(!sizeof(cmd)) cmd=({"cmd.exe"});

  string ret="";
  object(Stdio.File) f=Stdio.File();

  // werror("RNT: %{%O, %}\n", cmd);

  switch(getenv("REMOTE_METHOD"))
  {
    default:
      werror("Unknown REMOTE method %s\n",getenv("REMOTE_METHOD"));
      break;

    case "wine":
    case "WINE":
#if 0
      if(!silent && !filter)
      {
	return Process.create_process(({"wine",cmd*" "}))->wait();
      }
      else
#endif
      {
	// Some special cases that are builtin into sprshd.
	switch(cmd[0]) {
	case "getenv":
	  cmd = ({ "cmd", "/c", "echo", "%" + cmd[1] + "%" });
	  break;
	}
	object o=f->pipe(Stdio.PROP_BIDIRECTIONAL | Stdio.PROP_IPC);
	cmd=({ "wine" }) + cmd;
//	werror("WINE %O\n",cmd);
	object proc=Process.create_process(cmd,
					   ([
					     "stdout":o,
					     "env":getenv() + ([
					       "WINEDEBUG":"-all",
					     ]),
					   ]) );
	destruct(o);
	while(1)
	{
	  string s=f->read(8192,1);
	  if(!s || !strlen(s)) break;
	  s=replace(s,"\r\n","\n");
	  if(!silent) write(s);
	  ret+=s;
	}
	if(filter) filter(ret);
	destruct(f);
	return proc->wait();
      }

    case "wsl":
    case "WSL":
      {
	if (cmd[0] == "getenv") {
	  string value = getenv(cmd[1]) || getenv(lower_case(cmd[1])) || "";
	  if (filter) filter(value);
	  if (!silent) write(value + "\n");
	  return 0;
	}
	if (!no_fixpath) fix_paths_in_arglist(cmd);
	if (has_suffix(lower_case(cmd[0]), ".bat") ||
	    has_suffix(lower_case(cmd[0]), ".cmd"))
	  error("Run batch files through cmd.exe /c in WSL mode.\n");
	if (!has_suffix(lower_case(cmd[0]), ".exe")) cmd[0] += ".exe";

	object o = f->pipe(Stdio.PROP_BIDIRECTIONAL | Stdio.PROP_IPC);
	object proc = Process.create_process(cmd, (["stdout":o]));
	destruct(o);
	while (1) {
	  string s = f->read(8192, 1);
	  if (!s || !sizeof(s)) break;
	  s = replace(s, "\r\n", "\n");
	  if (!silent) write(s);
	  ret += s;
	}
	if (filter) filter(ret);
	destruct(f);
	return proc->wait();
      }

    case "ssh":
#if 0
      if(!silent && !filter)
      {
	return Process.create_process(({"ssh", getenv("NTHOST"), cmd*" "}))->wait();
      }
      else
#endif
      {
	// Some special cases that are builtin into sprshd.
	switch(cmd[0]) {
	case "getenv":
	  cmd = ({ "cmd", "/c", "echo", "%" + cmd[1] + "%" });
	  break;
	}
	object o=f->pipe(Stdio.PROP_BIDIRECTIONAL | Stdio.PROP_IPC);
	cmd=({ "ssh", getenv("NTHOST"), map(cmd, quote_ssh_arg) * " " });
	object proc=Process.create_process(cmd,
					   ([
					     "stdout":o,
					     "env":getenv() + ([
					     ]),
					   ]) );
	destruct(o);
	while(1)
	{
	  string s=f->read(8192,1);
	  if(!s || !strlen(s)) break;
	  s=replace(s,"\r\n","\n");
	  if(!silent) write(s);
	  ret+=s;
	}
	if(filter) filter(ret);
	destruct(f);
	return proc->wait();
      }

    case "cygwin":
    case "CYGWIN":
    {
      mapping env=copy_value(getenv());
      if(string tmp=getenv("REMOTE_VARIABLES"))
      {
	foreach(tmp/"\n",string var)
	  {
	    if(search(var,"=")!=-1 && sscanf(var,"%s=%s",string key, string val))
	    {
	      // Magic
	      if(!env[key])
	      {
		if(env[lower_case(key)])
		  key=lower_case(key);
		else if(env[upper_case(key)])
		  key=upper_case(key);
		else
		{
		  foreach(indices(env), string x)
		    {
		      if(lower_case(x) == lower_case(key))
		      {
			key=x;
			break;
		      }
		    }
		}
	      }
	      if(val[0]==';')
	      {
		env[key]=opt_path(env[key], val);
	      }
	      else if(val[-1]==';')
	      {
		env[key]=opt_path(val, env[key]);
	      }
	      else
	      {
		env[key]=val;
	      }
//    werror("%s = %s\n",key,env[key]);
	    }
	  }
      }

      if (!no_fixpath) fix_paths_in_arglist (cmd);

	object o=f->pipe(Stdio.PROP_IPC);
	object proc=Process.create_process(cmd,
					   (["stdout":o,"env":env]));
	destruct(o);
	while(1)
	{
	  string s=f->read(8192,1);
	  if(!s || !strlen(s)) break;
	  s=replace(s,"\r\n","\n");
	  if(!silent) write(s);
	  ret+=s;
	}
	if(filter) filter(ret);
	destruct(f);
	return proc->wait();
      }

    case 0:
    case "sprsh":
    case "SPRSH":
      // These sprshd builtins never read stdin or handle its OOB EOF.
      // Sending input to them can reset the Windows connection at close,
      // losing the last part of the reply (notably the exit status).
      int forward_stdin = !has_value(({ "copy", "mkdir", "getenv",
                                        "sprshd-ping" }), lower_case(cmd[0]));
      array vars=({"__handles_stderr=1"});

	/* This is somewhat experimental - Hubbe */
	if(!silent &&
	   !!Stdio.stdin->tcgetattr() &&
	   !!Stdio.stdout->tcgetattr() &&
	   getenv("TERM") != "dumb" &&
	   getenv("TERM") != "emacs")
	{
	  vars+=({ "TERM=sprsh" });
	}else{
	  vars+=({ "TERM=dumb" });
	}

      if(string tmp=getenv("REMOTE_VARIABLES"))
      {
	foreach(tmp/"\n",string var)
	  if(search(var,"=")!=-1)
	    vars+=({var});
      }
      cmd=vars+cmd;

      if (!no_fixpath) fix_paths_in_arglist (cmd);
      cmd = ({fixpath (getcwd())}) + cmd;

      if(!f->connect(getenv("NTHOST"),(int)getenv("NTPORT")))
      {
	werror("Failed to connect to "+getenv("NTHOST")+":"+getenv("NTPORT")
               +"  "+strerror(errno())+".\n");
	exit(1);
      }


      class SimpleInOut
      {
	object o=Stdio.File("stdin");
	string read() {
	  string s=o->read(1000,1);
	  if(!strlen(s)) return 0;
	  return s;
	}
	int write(string s)
	  {
	    return Stdio.stdout->write(s);
	  }

	int werr(string s)
	  {
	    return Stdio.stderr->write(s);
	  }
      };

      object inout;

#if __VERSION__ > 0.699999 && constant(thread_create)
      class RLInOut
      {
	Thread.Mutex m=Thread.Mutex();
	object rl=Stdio.Readline();
	string prompt="";

	array from_handler= ({"\r\n","\n\r","\n"});
	array to_handler= ({"\n\r","\n\r","\n\r"});

	string read()
	  {
	    string tmp=rl->read();
	    if(tmp)
	    {
	      tmp+="\n";
	      prompt="";
	    }
	    return tmp;
	  }

	int write(string s)
	  {
	    /* This provides some very basic terminal emulation */
	    /* We need to understand \r, \n, and \b */
//	    werror("%O\n",s);
	    rl->set_prompt("");
//	    Stdio.append_file("/tmp/liblog",sprintf("%O\n",s));
	    s=replace(prompt+s,from_handler,to_handler);
	    
	    array tmp=s/"/b";

	    array lines=s/"\r";
	    foreach(lines, string l)
	    {
	      if(strlen(l) && l[-1]=='\n')
	      {
		rl->write(l);
		prompt="";
	      }else{
		prompt=l;
	      }
	    }
	      
	    rl->set_prompt(prompt[..78]);
	    return strlen(s);
	  }

	int werr(string s) { return write(s); }

        protected void create()
	  {
	    rl->enable_history(512);
	    for(int e=0;e<256;e++)
	    {
	      switch(e)
	      {
		case '\n': case '\r': break;
		default:
		  from_handler+=({ sprintf("%c\b",e) });
		  to_handler+=({ sprintf("") });
	      }
	    }
	  }
      };

      if(!silent &&
	 !!Stdio.stdin->tcgetattr() &&
	!!Stdio.stdout->tcgetattr())
      {
	inout=RLInOut();
      }else
#endif
	inout=SimpleInOut();
	
      f->write(sprintf("%4c",sizeof(cmd)));
      for(int e=0;e<sizeof(cmd);e++)
	f->write(sprintf("%4c%s",strlen(cmd[e]),cmd[e]));

//      if(f->proxy)
//	f->proxy(Stdio.File("stdin"));
//      else
//      werror("FNORD\n");
        if (forward_stdin)
	thread_create(lambda(object f)
		      {
			int killed;
			int last_killed;
			void killme()
			  {
			    if(last_killed == time()) return;
			    last_killed=time();
			    werror("\r\n** Interrupted\r\n");
			    if(!killed || killed+5 > time())
			    {
			      f->write_oob("x");
			      if(!killed)
				killed=time();
			    }else{
			      exit(1);
			    }
			  };

			if(f->write_oob)
			{
			  signal(signum("SIGINT"), killme);
			}

			while(string s=inout->read())
			  f->write(s);

                        if (f->write_oob) {
                          // Send an EOF.
                          f->write_oob("\4");
                        }

			signal(signum("SIGINT"), 0);
			// Some port forwarders doesn't handle unidirectional
			// close too well and closes the connection completely
			// instead. Afaict, the one in VirtualBox 3.0.4 is one
			// of them (c.f. http://www.virtualbox.org/ticket/4925).
			//f->close("w");
		      },f);

      string sout = "", serr = "";
      while(1)
      {
        string s = read_sprsh_reply(f, 4, "output header");
	sscanf(s,"%c%3c",int channel, int len);
	if(!len) break;
        s = read_sprsh_reply(f, len, "output data");
	s=replace(s,"\r\n","\n");
	if(!silent)
	{
	  switch(channel)
	  {
	    case 0: /* Backwards compatibility */
	    case 1:  inout->write(s); break; // stdout
	    case 2:  inout->werr(s);  break; // stderr
	    default: /* reserved for future use */
	  }
	}
	if(filter)
	  switch (channel) {
	    case 0: ret += s; break;
	    case 1: sout += s; break;
	    case 2: serr += s; break;
	  }
      }
      if(filter) {
	if (ret != "" && sout != "") {
	  werror ("Strange sprshd is sending on both channel 0 and 1.\n");
	  exit (1);
	}
	if (ret != "") filter(ret);
	if (sout != "") filter(sout, 1);
	if (serr != "") filter(serr, 2);
      }
      sscanf(read_sprsh_reply(f, 4, "exit status"), "%4c", int code);
      f->close("r");
//      f->close("w");
//      werror("Closing stdout.\n");
      destruct(f);
      if(code > 255)
      {
	werror("Software failure. Press left mouse button to continue.\r\n"
	       "     Guru meditation #%s: %s\r\n",
	       sprintf("%016X",code)/8*".", explain(code));
	code=127;
      }
      return code;
  }
}

string tmp;

string popen_cmd(array(string) cmd)
{
  tmp="";
  silent_do_cmd(cmd,lambda(string x) { tmp=x; }, 1);
  return tmp;
}

string getntenv(string var)
{
  string s="";
  switch(getenv("REMOTE_METHOD"))
  {
    default:
      werror("Unknown REMOTE method %s\n",getenv("REMOTE_METHOD"));
      break;

    case "wine":
    case "WINE":
    case "wsl":
    case "WSL":
      return getenv(var) || getenv(lower_case(var));

    case 0:
    case "sprsh":
    case "SPRSH":
      return popen_cmd( ({"getenv",var}) );
  }
}


int do_cmd(array(string) cmd, mixed|void filter, void|int no_fixpath)
{
  return silent_do_cmd(cmd,filter, 0, no_fixpath);
}

string find_lib_location()
{
  return __FILE__;
}

string find_next_in_path(string|zero argv0, string cmd)
{
  string full_argv0 = combine_path(getcwd(),argv0);
  if(file_stat(full_argv0))
  {
    foreach((getenv("PATH")||"")/":",string x)
      {
	string fname=combine_path(getcwd(),x,cmd);
	if(mixed s=file_stat(fname))
	{
	  if(argv0)
	  {
	    if(full_argv0==fname)
	      argv0=0;
	  }else if (full_argv0 != fname) {
	    return fname;
	  }
	}
      }
  }else{
    foreach((getenv("PATH")||"")/":",string x)
      {
	string fname=combine_path(getcwd(),x,cmd);
	if(mixed s=file_stat(fname))
	{
	  if(Stdio.File(fname,"r")->read(2)=="#!")
	    continue;
	  return fname;
	}
      }

    foreach((getenv("PATH")||"")/":",string x)
    {
      string fname=combine_path(getcwd(),x,cmd);
      if(mixed s=file_stat(fname))
	return fname;
    }
  }

  return "/bin/"+cmd;
}
