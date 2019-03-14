// Not finished - Fredrik Hubinette

#pike __REAL_VERSION__

//! NNTP - The Network News Transfer Protocol.

//! helper class for protocol implementations.
//! @seealso
//! 	@[protocol]
class protocolhelper
{
  //! gets the result message supplied by the server for the last response
  string get_response_message()
  {
    return this->rest;
  }

}

//! Synchronous NNTP protocol
class protocol
{
  inherit Stdio.FILE : sock;
  inherit protocolhelper;

  string rest;

  //! reads the server result code for last request
  //!  used internally by command().
  int readreturncode()
  {
    int space, code;
    string r;
    rest=0;

    do {
      space=' ';
      string tmp=sock::gets();
      if(!tmp) return 0;
      sscanf(tmp,"%d%c%s",code,space,r);
      rest+=r;
    } while(space == '-');
    return code;
  }

  //! reads the message from the server as an array of lines
  array(string) read_body_lines()
  {
    array(string) ret=({});
    string s;
    while(s = sock::gets())
    {
      if(s=="." || s==".\r") return ret;
      sscanf(s,".%s",s);
      ret+=({s});
    }
    error("Connection closed by server.\n");
  }

  //! reads the message from the server
  string readreturnbody()
  {
    array(string) tmp=read_body_lines();
    return tmp*"\n"+"\n";
  }

  //! send the body of a message to the server.
  void writebody(string s)
  {
    s=replace(s,"\r","");
    foreach(s/"\n",string line)
      {
	if(sizeof(line) && line[0]=='.')
	  line="."+line+"\r\n";
	else
	  line=line+"\r\n";
	if(sock::write(line) != sizeof(line))
	  error("Failed to write body.\n");
      }
    sock::write(".\r\n");
  }

  //! send a command to the server
  //! @returns
  //!   the result code sent by the server
  int command(string cmd)
  {
    sock::write(cmd+"\r\n");
    return readreturncode();
  }

  //! send a command and require an ok response (200 series).
  //! throws an error if the command result was not success.
  int failsafe_command(string cmd)
  {
    if(command(cmd)/100 != 2)
      error(cmd+" failed.\n");
  }

  //! send a command that should return a message body.
  //!
  //! @returns
  //!  the message body
  string do_cmd_with_body(string cmd)
  {
    failsafe_command(cmd);
    return readreturnbody();
  }
}

//! Asynchronous NNTP protocol
class asyncprotocol
{
  inherit Stdio.File : sock;
  inherit protocolhelper;

  string rest;
  string inbuf, outbuf = "";
  array next_cbs = ({ });

  void io_error(string fmt, mixed ... args) {
    error(fmt, @args);
  }

  int(0..1) is_ip(string iporhost)
  {
     int i1, i2, i3, i4;

     if(sscanf(iporhost, "%d.%d.%d.%d", i1, i2, i3, i4) == 4)
	if(max(i1, i2, i3, i4) <= 255 && min(i1, i2, i3, i4) >= 0)
	   return 1;

     return arrayp(Protocols.IPv6.parse_addr(iporhost));
  }

  int async_connect(string to, int port, function cb, mixed ... extra)
  {
    return ::async_connect(to, port, lambda(int succ) {
			     if(succ)
			     {
			       set_nonblocking(read_cb, write_cb, close_cb);
			       return cb(succ, @extra);
			     } else
			     {
			       io_error("Failed to connect.\n");
			     }
			   });
  }

  int read_cb(mixed id, string data)
  {
     if(sizeof(data))
        if(!inbuf)
           inbuf = data;
        else
	   inbuf += data;

     if(sizeof(next_cbs))
     {
	int pos;

	if((pos = search(inbuf, "\n")) != -1)
	{
	  call_out(next_cbs[0], 0, inbuf[..pos -1], @next_cbs[1]);
	  inbuf = inbuf[pos + 1..];
	}

	next_cbs = next_cbs[2..];
     }

     return 0;
  }

  void get_next_line(function cb, mixed ... extra)
  {
     if(sizeof(next_cbs))
        next_cbs += ({ cb, extra });
     else
     {
	int pos;

	if(inbuf && (pos = search(inbuf, "\n")) != -1)
	{
	   call_out(cb, 0, inbuf[..pos - 1], @extra);
	   inbuf = inbuf[pos + 1..];
	}
	else
           next_cbs = ({ cb, extra });
     }
  }

  int write_cb(mixed id)
  {
     if (outbuf)
     {
	int written = sock::write(outbuf);

	if (written == sizeof(outbuf))
	{
           outbuf = 0;
	} else
	{
	   outbuf = outbuf[written..];
	}

     }

     return 0;
  }

  int close_cb(mixed id)
  {
     io_error("Connection was closed.\n");

     return 0;
  }

  int write(string s, mixed ... args) {
    string tmp;

    tmp = s;
    if(sizeof(args)) tmp = sprintf(s, @args);

    if(outbuf)
      outbuf += tmp;
    else
    {
      int written;

      outbuf = tmp;
      written = sock::write(outbuf);

      if(written == sizeof(outbuf))
	 outbuf = 0;
      else
	 outbuf = outbuf[written..];
    }

    return sizeof(tmp);
  }

  //! reads the server result code for last request
  //!  used internally by command().
  void readreturncode(function cb, mixed ... extra)
  {
     get_next_line(iteratereturncode, cb, extra);
     rest=0;
  }

  void iteratereturncode(string tmp, function cb, array extra)
  {
    int space, code;
    string r;

    //Stdio.stdout->write("::: %O\n", tmp);

    space=' ';

    if(!sizeof(tmp))
    {
      if(cb) cb(0,@extra);
      return;
    }

    sscanf(tmp,"%d%c%s",code,space,r);
    rest+=r;

    if(space != '-')
    {
      if(cb) cb(code,@extra);
      return;
    }

    get_next_line(iteratereturncode, cb, extra);
  }

  //! send a command to the server
  //! @returns
  //!   the result code sent by the server
  void command(string cmd, function|void cb)
  {
    write(cmd+"\r\n");
    readreturncode(cb);
  }
}

//! An NNTP client
class client
{
  inherit protocol;

  class Group
  {
    string group;
    int min;
    int max;
  }

  //! Returns a list of all active groups.
  array(Group) list_groups()
  {
    array(Group) ret=({});
    failsafe_command("list active");
    foreach(read_body_lines(),string line)
    {
      Group o=Group();
      if(sscanf(line,"%s %d %d",o->group,o->max,o->min)==3)
	ret+=({o});
    }

    return ret;
  }

  //! The current news group.
  Group current_group;

  //! Sets the current news group to @[o].
  void set_group(Group o)
  {
    if(current_group==o)
      return;
    failsafe_command("group "+o->group);
    current_group=o;
  }

  //! Sets the current group to @[group].
  Group go_to_group(string group)
  {
    failsafe_command("group "+group);
    Group o=Group();
    o->group=group;
    sscanf(rest,"%d %d %d",int num,o->min,o->max);
    current_group=o;
    return o;
  }

  //!
  string head(void|int|string x)
  {
    failsafe_command("head"+(x?" "+x:""));
    return readreturnbody();
  }

  //!
  string body(void|int|string x)
  {
    failsafe_command("body"+(x?" "+x:""));
    return readreturnbody();
  }

  //!
  string article(void|int|string x)
  {
    failsafe_command("article"+(x?" "+x:""));
    return readreturnbody();
  }

  //! @param server
  //!   NNTP server to connect to.
  //!   Defaults to the server specified by
  //!   the environment variable @expr{NNTPSERVER@}.
  void create(string|void server)
  {
    if(!server)
    {
      server=getenv("NNTPSERVER");

      if(!server)
      {
	// FIXME: Check /etc/nntpserver here
      }
    }

    if(!connect(server,119))
      error("Failed to connect to news server.\n");

    if(readreturncode()/100 != 2)
      error("Connection refused by NNTP server.\n");

    if(command("mode reader")/100 !=2)
      error("Mode reader failed.\n");
  }
}
