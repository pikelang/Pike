#pike __REAL_VERSION__

class protocol
{
  // Maybe this should be the other way around?
  inherit .NNTP.protocol;
}

//!
class client
{
  inherit protocol;

  //! A mapping(int:string) that maps SMTP return
  //! codes to english textual messages.
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
         "[E.g., mailbox syntax incorrect or relaying denied]",
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

  //! @decl void create()
  //! @decl void create(Stdio.File server)
  //! @decl void create(string server, void|int port)
  //! Creates an SMTP mail client and connects it to the
  //! the @[server] provided. The server parameter may
  //! either be a string witht the hostnam of the mail server,
  //! or it may be a file object acting as a mail server.
  //! If @[server] is a string, than an optional port parameter
  //! may be provided. If no port parameter is provided, port
  //! 25 is assumed. If no parameters at all is provided
  //! the client will look up the mail host by searching
  //! for the DNS MX record.
  //!
  //! @throws
  //!   Throws an exception if the client fails to connect to
  //!   the mail server.
  void create(void|string|Stdio.File server, int|void port)
  {
    if(!server)
    {
      // Lookup MX record here (Using DNS.pmod)
      object dns=master()->resolv("Protocols")["DNS"]->client();
      server=dns->get_primary_mx(gethostname());
    }

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
  
  //! Sends a mail message from @[from] to the mail addresses
  //! listed in @[to] with the mail body @[body]. The body
  //! should be a correctly formatted mail DATA block, e.g.
  //! produced by @[MIME.Message].
  //!
  //! @seealso
  //!   @[simple_mail]
  //!
  //! @throws
  //!   If the mail server returns any other return code than
  //!   200-399 an exception will be thrown.
  void send_message(string from, array(string) to, string body)
  {
    cmd("MAIL FROM: <" + from + ">");
    foreach(to, string t) {
      cmd("RCPT TO: <" + t + ">");
    }
    cmd("DATA");

    // Perform quoting according to RFC 2821 4.5.2.
    if (sizeof(body) && body[0] == '.') {
      body = "." + body;
    }
    body = replace(body, "\r\n.", "\r\n..");

    // RFC 2841 4.1.1.4:
    //   An extra <CRLF> MUST NOT be added, as that would cause an empty
    //   line to be added to the message.
    if (has_suffix(body, "\r\n")) {
      body += ".";
    } else {
      body += "\r\n.";
    }
    cmd(body);
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

  //! Sends an e-mail. Wrapper function that uses @[send_message].
  //!
  //! @throws
  //!   If the mail server returns any other return code than
  //!   200-399 an exception will be thrown.
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

  //! Verifies the mail address @[addr] against the mail server.
  //!
  //! @returns
  //!   @array
  //!     @elem int code
  //!       The numerical return code from the VRFY call.
  //!     @elem string message
  //!       The textual answer to the VRFY call.
  //!  @endarray
  //!
  //! @note
  //!   Some mail servers does not answer truthfully to
  //!   verfification queries in order to prevent spammers
  //!   and others to gain information about the mail
  //!   addresses present on the mail server.
  //!
  //! @throws
  //!   If the mail server returns any other return code than
  //!   200-399 an exception will be thrown.
  array(int|string) verify(string addr)
  {
    return ({command("VRFY "+addr),rest});
  }
}
