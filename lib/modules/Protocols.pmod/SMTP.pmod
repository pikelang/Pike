#pike __REAL_VERSION__

class protocol
{
  // Maybe this should be the other way around?
  inherit .NNTP.protocol;
}

class client
{
  inherit protocol;

  constant reply_codes =
  ([ 211:"System status, or system help reply",
     214:"Help message",
     220:"<host> Service ready",
     221:"<host> Service closing transmission channel",
     250:"Requested mail action okay, completed",
     251:"User not local; will forward to <forward-path>",
     354:"Start mail input; end with <CRLF>.<CRLF>",
     421:"<host> Service not available, closing transmission channel "
         "[This may be a reply to any command if the service knows it "
         "must shut down]",
     450:"Requested mail action not taken: mailbox unavailable "
         "[E.g., mailbox busy]",
     451:"Requested action aborted: local error in processing",
     452:"Requested action not taken: insufficient system storage",
     500:"Syntax error, command unrecognized "
         "[This may include errors such as command line too long]",
     501:"Syntax error in parameters or arguments",
     502:"Command not implemented",
     503:"Bad sequence of commands",
     504:"Command parameter not implemented",
     550:"Requested action not taken: mailbox unavailable "
         "[E.g., mailbox not found, no access]",
     551:"User not local; please try <forward-path>",
     552:"Requested mail action aborted: exceeded storage allocation",
     553:"Requested action not taken: mailbox name not allowed "
         "[E.g., mailbox syntax incorrect]",
     554:"Transaction failed" ]);

  static private int cmd(string c, string|void comment)
  {
    int r = command(c);
    switch(r) {
    case 200..399:
      break;
    default:
      throw(({"SMTP: "+c+"\n"+(comment?"SMTP: "+comment+"\n":"")+
	      "SMTP: "+reply_codes[r]+"\n", backtrace()}));
    }
    return r;
  }

  void create(void|string|Stdio.File server, int|void port)
  {
    if(!server)
    {
      // Lookup MX record here (Using DNS.pmod)
      object dns=master()->resolv("Protocols")["DNS"]->client();
      server=dns->get_primary_mx(gethostname());
    }

    werror("server=%O\n",server);
    if (objectp(server))
       assign(server);
    else
    {
       if(!port)
	  port = 25;

       if(!server || !connect(server, port))
       {
	  throw(({"Failed to connect to mail server.\n",backtrace()}));
       }
    }

    if(readreturncode()/100 != 2)
      throw(({"Connection refused by SMTP server.\n",backtrace()}));

    if(catch(cmd("EHLO "+gethostname())))
      cmd("HELO "+gethostname(), "greeting failed.");
  }
  
  void send_message(string from, array(string) to, string body)
  {
    cmd("MAIL FROM: <" + from + ">");
    foreach(to, string t) {
      cmd("RCPT TO: <" + t + ">");
    }
    cmd("DATA");
    cmd(body+"\r\n.");
    cmd("QUIT");
  }

  static string parse_addr(string addr)
  {
    array(string|int) tokens = replace(MIME.tokenize(addr), '@', "@");

    int i;
    tokens = tokens[search(tokens, '<') + 1..];

    if ((i = search(tokens, '>')) != -1) {
      tokens = tokens[..i-1];
    }
    return tokens*"";
  }

  void simple_mail(string to, string subject, string from, string msg)
  {
    if (search(msg,"\r\n")==-1)
      msg=replace(msg,"\n","\r\n"); // *simple* mail /Mirar
    send_message(parse_addr(from), ({ parse_addr(to) }),
		 (string)MIME.Message(msg, (["mime-version":"1.0",
					     "subject":subject,
					     "from":from,
					     "to":to,
					     "content-type":
					       "text/plain;charset=iso-8859-1",
					     "content-transfer-encoding":
					       "8bit"])));
  }

  array(int|string) verify(string addr)
  {
    return ({command("VRFY "+addr),rest});
  }
}
