#if !constant(strerror)
#define strerror(X) X
#endif

#ifdef __NT__
void exece(string cmd, array(string) args)
{
  exit(Process.create_process( ({ cmd }) + args )->wait());
}
#endif

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

string fixpath(string s)
{
  string mnt=getenv("NTMOUNT");
  s=follow_symlinks(s);
  if(mnt && strlen(mnt)) s=replace(s,mnt,"");
  return replace(s,"/","\\");
}


string fixabspath(string s)
{
  return replace(s,"/","\\");
}

string opt_path(string p1, string p2)
{
  return  ( ( ((p1||"") + ";" + (p2||"")) / ";" ) - ({""}) ) * ";";
}


int silent_do_cmd(string *cmd, mixed|void filter, int|void silent)
{
//  werror("%O\n",cmd);

  if(!sizeof(cmd)) cmd=({"cmd.exe"});

  string ret="";
  object(Stdio.File) f=Stdio.File();

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
	object o=f->pipe(Stdio.PROP_BIDIRECTIONAL | Stdio.PROP_IPC);
	cmd=({"wine",
	      "-winver","win95",
	      "-debugmsg","fixme-all",
	      "-debugmsg","trace+all",
	      "-debugmsg","+relay",
	      cmd*" "});
//	werror("WINE %O\n",cmd);
	object proc=Process.create_process(cmd, (["stdout":o]) );
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

      string mnt=getenv("NTMOUNT");
#if 1
      /* Experimental */
      if(mnt && strlen(mnt)>1)
      {
	for(int e=1;e<sizeof(cmd);e++)
	  cmd[e]=replace(cmd[e],mnt,getenv("NTDRIVE"));
      }
#endif
      
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
      if(string tmp=getenv("REMOTE_VARIABLES"))
      {
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

	foreach(tmp/"\n",string var)
	  if(search(var,"=")!=-1)
	    vars+=({var});
	cmd=vars+cmd;
      }
      string tmp=getcwd();
      string mnt=getenv("NTMOUNT");
      if(mnt && strlen(mnt))
	tmp=replace(tmp,mnt,getenv("NTDRIVE"));
      else
	tmp=getenv("NTDRIVE")+tmp;
      
      tmp=replace(tmp,"/","\\");

      cmd=({ tmp })+cmd;

#if 1
      /* Experimental */
      if(mnt && strlen(mnt)>1)
      {
	for(int e=1;e<sizeof(cmd);e++)
	  cmd[e]=replace(cmd[e],mnt,getenv("NTDRIVE"));
      }
#endif

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

	void create()
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
	thread_create(lambda(object f)
		      {
			int killed;
			int last_killed;
			void killme()
			  {
			    if(last_killed == time()) return;
			    last_killed=time();
			    werror("\r\n** Interupted\r\n");
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

			signal(signum("SIGINT"), 0);
			f->close("w");
		      },f);

      while(1)
      {
	string s = f->read(4);
	if (!s) {
	  werror("Connection closed!\n");
	  exit(1);
	}
	sscanf(s,"%c%3c",int channel, int len);
	if(!len) break;
	s=f->read(len);
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
	if(filter) ret+=s;
      }
      if(filter) filter(ret);
      sscanf(f->read(4),"%4c",int code);
      f->close("r");
//      f->close("w");
//      werror("Closing stdout.\n");
      destruct(f);
      if(code > 255)
      {
	werror("Software failure. Press left mouse button to continue.\r\n"
	       "          Guru meditation #%s\r\n",
	       sprintf("%016X",code)/8*".");
	code=127;
      }
      return code;
  }
}

string tmp;

string popen_cmd(string *cmd)
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
      return getenv(var) || getenv(lower_case(var));

    case 0:
    case "sprsh":
    case "SPRSH":
      return popen_cmd( ({"getenv",var}) );
  }
}


int do_cmd(string *cmd, mixed|void filter)
{
  return silent_do_cmd(cmd,filter);
}

string find_lib_location()
{
  return __FILE__;
}

string find_next_in_path(string argv0,string cmd)
{
  argv0=combine_path(getcwd(),argv0);
  if(file_stat(argv0))
  {
    foreach((getenv("PATH")||"")/":",string x)
      {
	string fname=combine_path(getcwd(),x,cmd);
	if(mixed s=file_stat(fname))
	{
	  if(argv0)
	  {
	    if(argv0==fname)
	      argv0=0;
	  }else{
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
  }

  foreach((getenv("PATH")||"")/":",string x)
    {
      string fname=combine_path(getcwd(),x,cmd);
      if(mixed s=file_stat(fname))
	return fname;
    }

  return "/bin/"+cmd;
}
