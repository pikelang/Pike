#if !constant(strerror)
#define strerror(X) X
#endif

string fixpath(string s)
{
  string mnt=getenv("NTMOUNT");
  if(mnt && strlen(mnt)) s=replace(s,mnt,"");
  return replace(s,"/","\\");
}


string fixabspath(string s)
{
  return replace(s,"/","\\");
}

int silent_do_cmd(string *cmd, mixed|void filter, int|void silent)
{
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
	object proc=Process.create_process(cmd,(["stdout":o]));
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
	array vars=({});
	foreach(tmp/"\n",string var)
	  if(search(var,"=")!=-1)
	    vars+=({var});
	cmd=vars+cmd;
      }
      string tmp=getcwd();
      string mnt=getenv("NTMOUNT");
      if(mnt && strlen(mnt)) tmp=replace(tmp,mnt,"");
      cmd=({getenv("NTDRIVE")+replace(tmp,"/","\\")})+cmd;

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
	werror("Failed to connect "+strerror(errno())+".\n");
	exit(1);
	}
      
      f->write(sprintf("%4c",sizeof(cmd)));
      for(int e=0;e<sizeof(cmd);e++)
	f->write(sprintf("%4c%s",strlen(cmd[e]),cmd[e]));

//      if(f->proxy)
//	f->proxy(Stdio.File("stdin"));
//      else
//      werror("FNORD\n");
	thread_create(lambda(object f)
		      {
			object stdin=Stdio.File("stdin");
			while(string s=stdin->read(1000,1))
			{
			  if(!strlen(s)) break;
			  f->write(s);
			}
			f->close("w");
		      },f);

      while(1)
      {
	string s;
	sscanf(f->read(4),"%4c",int len);
	if(!len) break;
	s=f->read(len);
	s=replace(s,"\r\n","\n");
	if(!silent) write(s);
	ret+=s;
      }
      if(filter) filter(ret);
      sscanf(f->read(4),"%4c",int code);
      f->close("r");
//      f->close("w");
//      werror("Closing stdout.\n");
      destruct(f);
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
  werror("DOING "+cmd*" "+"\n");
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
	if(array s=file_stat(fname))
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
	if(array s=file_stat(fname))
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
      if(array s=file_stat(fname))
	return fname;
    }

  return "/bin/"+cmd;
}
