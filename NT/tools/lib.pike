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

string handle_input(object o)
{
  if(o->proxy)
    o->proxy(Stdio.File("stdin"));
  else
    thread_create(lambda(object o)
		 {
		   object stdin=Stdio.File("stdin");
		   while(string s=stdin->read(1000,1))
		     o->write(s);
		 },o);
}


object low_do_cmd(string *cmd)
{
  object(Stdio.File) f=Stdio.File();
  if(!f->connect(getenv("NTHOST"),(int)getenv("NTPORT")))
  {
    werror("Failed to connect "+strerror(errno())+".\n");
    exit(1);
  }

  string tmp=getcwd();
  string mnt=getenv("NTMOUNT");
  if(mnt && strlen(mnt)) tmp=replace(tmp,mnt,"");
  cmd=({getenv("NTDRIVE")+replace(tmp,"/","\\")})+cmd;
  f->write(sprintf("%4c",sizeof(cmd)));
  for(int e=0;e<sizeof(cmd);e++)
    f->write(sprintf("%4c%s",strlen(cmd[e]),cmd[e]));
  return f;
}

int silent_do_cmd(string *cmd, mixed|void filter)
{
  object(Stdio.File) f=low_do_cmd(cmd);

  handle_input(f);
  string ret="";
  while(1)
  {
    string s;
    sscanf(f->read(4),"%4c",int len);
    if(!len) break;
    write(s=f->read(len));
    ret+=s;
  }
  if(filter) filter(ret);
  sscanf(f->read(4),"%4c",int code);
  f->close("r");
  f->close("w");
  destruct(f);
  return code;
}

string getntenv(string var)
{
  string s="";
  object(Stdio.File) f=low_do_cmd( ({"getenv",var}) );
  
  while(1)
  {
    sscanf(f->read(4),"%4c", int len);
    if(!len) break;
    s+=f->read(len);
  }

  sscanf(f->read(4),"%4c",int code);
  f->close("r");
  f->close("w");
  destruct(f);
  return s;
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
