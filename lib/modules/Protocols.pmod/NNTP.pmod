// Not finished - Fredrik Hubinette

#pike __REAL_VERSION__

class protocol
{
  inherit Stdio.FILE : news;

  string rest;

  int readreturncode()
  {
    int space, code;
    do {
      space=' ';
      string tmp=news::gets();
      if(!tmp) return 0;
      sscanf(tmp,"%d%c%s",code,space,rest);
    } while(space == '-');
    return code;
  }

  array(string) read_body_lines()
  {
    array(string) ret=({});
    string s;
    while(s = news::gets())
    {
      if(s=="." || s==".\r") return ret;
      sscanf(s,".%s",s);
      ret+=({s});
    }
    error("NNTP: connection closed by news server.\n");
  }

  string readreturnbody()
  {
    array(string) tmp=read_body_lines();
    return tmp*"\n"+"\n";
  }

  void writebody(string s)
  {
    s=replace(s,"\r","");
    foreach(s/"\n",string line)
      {
	if(strlen(line) && line[0]=='.')
	  line="."+line+"\r\n";
	else
	  line=line+"\r\n";
	if(news::write(line) != strlen(line))
	  error("NNTP: Failed to write body.\n");
      }
    news::write(".\r\n");
  }

  int command(string cmd)
  {
    news::write(cmd+"\r\n");
    return readreturncode();
  }

  int failsafe_command(string cmd)
  {
    if(command(cmd)/100 != 2)
      error("NEWS "+cmd+" failed.\n");
  }

  string do_cmd_with_body(string cmd)
  {
    failsafe_command(cmd);
    return readreturnbody();
  }

};

class client
{
  inherit protocol;

  class Group
  {
    string group;
    int min;
    int max;
  }

  array(object(Group)) list_groups()
  {
    array(object(Group)) ret=({});
    failsafe_command("list active");
    foreach(read_body_lines(),string line)
      {
	object o=Group();
	if(sscanf(line,"%s %d %d",o->group,o->max,o->min)==3)
	  ret+=({o});
      }

    return ret;
    
  }

  object(Group) current_group;

  void set_group(object(Group) o)
  {
    if(current_group==o)
      return;
    failsafe_command("group "+o->group);
    current_group=o;
  }

  object(Group) go_to_group(string group)
  {
    failsafe_command("group "+group);
    object o=Group();
    o->group=group;
    sscanf(rest,"%d %d %d",int num,o->min,o->max);
    current_group=o;
    return o;
  }

  string head(void|int|string x)
  {
    failsafe_command("head"+(x?" "+x:""));
    return readreturnbody();
  }

  string body(void|int|string x)
  {
    failsafe_command("body"+(x?" "+x:""));
    return readreturnbody();
  }

  string article(void|int|string x)
  {
    failsafe_command("article"+(x?" "+x:""));
    return readreturnbody();
  }

  void create(string|void server)
  {
    if(!server)
    {
      server=getenv("NNTPSERVER");

      if(!server)
      {
	// Check /etc/nntpserver here 
      }
    }

    if(!connect(server,119))
      error("Failed to connect to news server.\n");

    if(readreturncode()/100 != 2)
      error("Connection refused by NNTP server.\n");

    if(command("mode reader")/100 !=2)
      error("NNTP: mode reader failed.\n");
    
  }
};
