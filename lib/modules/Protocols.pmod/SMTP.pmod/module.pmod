#pike __REAL_VERSION__

//! A mapping(int:string) that maps SMTP return
//! codes to english textual messages.
mapping(int:string) replycodes =
([ 211:"System status, or system help reply",
   214:"Help message",
   220:"<host> Service ready",
   221:"<host> Service closing transmission channel",
   250:"Requested mail action okay, completed",
   251:"User not local; will forward to <forward-path>",
   252:"Cannot VRFY user, but will accept message and attempt delivery",
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


class Protocol
{
  // Maybe this should be the other way around?
  inherit Protocols.NNTP.protocol;
}

//!
class Client
{
  inherit Protocol;

  static private int cmd(string c, string|void comment)
  {
    int r = command(c);
    switch(r) {
    case 200..399:
      break;
    default:
      error( "SMTP: "+c+"\n"+(comment?"SMTP: "+comment+"\n":"")+
	     "SMTP: "+replycodes[r]+"\n" );
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
	 error("Failed to connect to mail server.\n");
       }
    }

    if(readreturncode()/100 != 2)
      error("Connection refused by SMTP server.\n");

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

    // RFC 2821 4.1.1.4:
    //   An extra <CRLF> MUST NOT be added, as that would cause an empty
    //   line to be added to the message.
    if (has_suffix(body, "\r\n"))
      body += ".";
    else
      body += "\r\n.";

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
  //! @note
  //!   Some important headers are set to:
  //!   @expr{"Content-Type: text/plain; charset=iso-8859-1"@} and 
  //!   @expr{"Content-Transfer-Encoding: 8bit"@}. @expr{"Date:"@}
  //!   header isn't used at all.
  //!
  //! @throws
  //!   If the mail server returns any other return code than
  //!   200-399 an exception will be thrown.
  void simple_mail(string to, string subject, string from, string msg)
  {
    if (!has_value(msg, "\r\n"))
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

//! Class to store configuration variable for the SMTP server
class Configuration {

  //! Message max size
  int maxsize = 100240000;
  
  //! Maximum number of recipients
  int maxrcpt = 1000;
  
  //! Verify sender domain for MX
  int checkdns = 0;
  
  //! Lamme check email from validity
  int checkemail = 1; 

  //! Give raw data and normal MIME data, if set to
  //! yes your cb_data function should take an extra
  //! string argument
  int givedata = 1;

  // the domains for each i relay
  array(string) domains = ({});

  // the callback functions used to guess if user is ok or not
  function cb_rcptto;
  function cb_data;
  function cb_mailfrom;

  void create(array(string) _domains, void|function _cb_mailfrom,
	      void|function _cb_rcptto, void|function _cb_data) {
    foreach(_domains, string domain)
      domains += ({ lower_case(domain) });

    cb_mailfrom = _cb_mailfrom;
    cb_rcptto = _cb_rcptto;
    cb_data = _cb_data;
  }

  array(string) get_features() {
    return ({ "PIPELINING", "8BITMIME", "SIZE " + maxsize });
  }
};

//! The low-level class for the SMTP server
class Connection {

  static Configuration cfg;

  // The commands this module supports
  mapping(string:function) commands = ([
    "ehlo" : ehlo,
    "helo" : helo,
    "mail" : mail,
    "rcpt" : rcpt,
    "data" : data,
    "rset" : rset,
    "vrfy" : vrfy,
    "quit" : quit,
    "noop" : noop,
  ]);

  constant protocol = "ESMTP";

  // the fd of the socket
  static Stdio.File fd = Stdio.File();
  // the input buffer for read_cb
  static string inputbuffer = "";
  // the size of the old data string in read_cb
  static int sizeofpreviousdata = 0;
  // the from address
  static string mailfrom = "";
  // to the address(es)
  static array(string) mailto = ({ });
  // the ident we get from ehlo/helo
  static string ident = "";
  // these are obvious
  static string remoteaddr, localaddr;
  static int localport;
  // my name
  static string localhost = gethostname();

  // the sequence of commands the client send
  static array(string) sequence = ({ });
  // the message id of the current mail
  private string|int messageid;
  
  // whether you are in data mode or not...
  int datamode = 0;

   static void handle_timeout(string cmd)
   {
     string errmsg = "421 Error: timeout exceeded after command " +
       cmd || "unknown command!" + "\r\n";
     catch(fd->write(errmsg));
     log(errmsg);
     shutdown_fd();
   }

   // return true if the given return code from the call back function
   // is a success one or not
   static int is_success(array|int check)
   {
     return (getretcode(check)/100 == 2);
   }

   // get the return code from the callback function
   static int getretcode(array|int check)
   {
     int smtpretcode;
     if(arrayp(check))
       smtpretcode = check[0];
     else
       smtpretcode = check;
     // if the callback function return nothing, fail the SMTP command
     if(!smtpretcode)
       return 451;
     return smtpretcode;
   }

   // get optionnal error string from the callback function
   // 0 is no error string were returned
   static int|string geterrorstring(array|int check)
   {
     if(arrayp(check) && stringp(check[1]))
       return check[1];
     return 0;
   }
   
   static void outcode(int code, void|string internal_error)
   {
     string msg = sprintf("%d %s", code, replycodes[code]);
     if(internal_error)
       msg += ". Problem due to the internal error: " + internal_error;
     msg += "\r\n";
     fd->write(msg);
#ifdef SMTP_DEBUG
     log(msg);
#endif
   }

  //! This function is called whenever the SMTP server logs something.
  //! By default the log function is @[werror].
   function(string:mixed) logfunction = werror;

   static void log(string fmt, mixed ... args)
   {
     string errmsg = Calendar.now()->format_time() + 
       " Pike "+protocol+" server : ";
     if(messageid)
       errmsg += messageid + ": ";
     errmsg += fmt + "\n";
     if(args && sizeof(args) > 0)
       logfunction(sprintf(errmsg, @args));
     else
       logfunction(errmsg);
   }
  
   // make the received header
   static string received()
   {
     string remotehost =
        Protocols.DNS.client()->gethostbyaddr(remoteaddr)[0]
     	|| remoteaddr;
     string rec;
     rec=sprintf("from %s (%s [%s]) by %s (Pike "+protocol+
		 " server) with "+protocol+" id %d ; %s",
       ident, remotehost, remoteaddr,
       gethostname(), messageid, Calendar.now()->format_smtp());
     return rec;
   }
   
   void helo(string argument)
   {
     remove_call_out(handle_timeout);
     call_out(handle_timeout, 310, "HELO");
     if(sizeof(argument) > 0)
     {
       fd->write("250 %s\r\n", localhost);
       ident = argument;
#ifdef SMTP_DEBUG
       log("helo from %s", ident);
#endif
       sequence += ({ "helo" });
     }
     else
       outcode(501);
   }

   void ehlo(string argument)
   {
     remove_call_out(handle_timeout);
     call_out(handle_timeout, 310, "EHLO");
     if(sizeof(argument) > 0)
     {
       string out = "250-" + localhost + "\r\n";
       int i = 0;
       for(; i < sizeof(features) - 1; i++)
       {
         out += "250-" + features[i] + "\r\n";
       }
       out += "250 " + features[i] + "\r\n";
       fd->write(out);
       sequence += ({ "ehlo" });
       ident = argument;
#ifdef SMTP_DEBUG
       log("helo from %s", ident);
#endif
     }
     else
      outcode(501);
   }

   void lhlo(string argument)
   {
     ehlo(argument);
   }
 
   // fetch the email address from the mail from: or rcpt to: commands
   // content: the input line like mail from:<toto@caudium.net>
   // what: the action either from or to
   int|string parse_email(string content, string what)
   {
      array parts = content / ":";
      if(lower_case(parts[0]) != what)
        return 500;
      string validating_mail;
      parts[1] = String.trim_all_whites(parts[1]);
      if(!sscanf(parts[1], "<%s>", validating_mail))
        sscanf(parts[1], "%s", validating_mail);
      if(validating_mail == "")
        validating_mail = "MAILER-DAEMON@" + cfg->domains[0];
      array emailparts = validating_mail / "@";
      array(string) temp = lower_case(emailparts[1]) / ".";
      string domain = temp[sizeof(temp)-2..] * ".";
      if(cfg->checkemail && sizeof(emailparts) != 2)
      {
        log("invalid mail address '%O', command=%O\n", emailparts, what);
        return 553;
      }
      if(cfg->checkdns)
      {
        write("checking dns\n");
        if(what == "from" && !Protocols.DNS.client()->get_primary_mx(domain))
        {
          log("check dns failed, command=%O, domain=%O\n", what, domain);
          return 553;
        }
      }
      if(what == "to" && !has_value(cfg->domains, domain) &&
	 !has_value(cfg->domains, "*") )
      {
        log("relaying denied, command=%O, cfg->domains=%O, domain=%O\n",
	    what, cfg->domains, domain);
        return 553;
      }
      return validating_mail;
   }
   
   void mail(string argument)
   {
     remove_call_out(handle_timeout);
     call_out(handle_timeout, 310, "MAIL FROM");
     int sequence_ok = 0;
     foreach(({ "ehlo", "helo", "lhlo" }), string needle)
     {
       if(has_value(sequence, needle))
         sequence_ok = 1;
     }
     if(sequence_ok)
     {
       mixed email = parse_email(argument, "from");
       if(intp(email))
         outcode(email);
       else
       {
         mixed err;
      	 int|array check;
         err = catch(check = cfg->cb_mailfrom(email));
         if(err)
         {
           outcode(451, err[0]);
           log(describe_backtrace(err));
           return;
         }
         if(is_success(check))
	 {
	   mailfrom = email;
	   mailto = ({ });
	   /* this is used to avoid this problem:
	   250 Requested mail action okay, completed
	   mail from: vida@caudium.net
	   250 Requested mail action okay, completed
	   rcpt to: toto@ece.Fr
	   250 Requested mail action okay, completed
	   mail from: vida@caudium.net
 	   250 Requested mail action okay, completed
	   rcpt to: tux@iteam.org
	   553 Requested action not taken: mailbox name not allowed
               [E.g., mailbox syntax incorrect or relaying denied]
	   data
	   354 Start mail input; end with <CRLF>.<CRLF>
           */
           sequence -= ({ "rcpt to" });
           sequence += ({ "mail from" });
         }
         outcode(getretcode(check), geterrorstring(check));
       }
     }
     else
       outcode(503);
   }
 
   void rcpt(string argument)
   {
     mixed err;
     remove_call_out(handle_timeout);
     call_out(handle_timeout, 310, "RCPT TO");
     if(!has_value(sequence, "mail from"))
     {
       outcode(503);
       return;
     }
     if(sizeof(mailto) >= cfg->maxrcpt)
     {
       outcode(552);
       return;
     }
     mixed email = parse_email(argument, "to");
     if(intp(email))
       outcode(email);
     else
     {
       int|array check;
       err = catch(check = cfg->cb_rcptto(email));
       if(err)
       {
         outcode(451);
         log(describe_backtrace(err));
         return;
       }
       if(is_success(check))
       {
         mailto += ({ email });
         sequence += ({ "rcpt to" });
       }
       outcode(getretcode(check), geterrorstring(check));
     }
   }
  
   void data(string argument)
   {
     remove_call_out(handle_timeout);
     call_out(handle_timeout, 610, "DATA");
     if(!has_value(sequence, "rcpt to"))
     {
       outcode(503);
       return;
     }
     datamode = 1;
     outcode(354);
   }

   MIME.Message format_headers(MIME.Message message)
   {
     messageid = hash(message->getdata()[..1000]) || random(100000);
     // first add missing headers
     if(!message->headers->to)
       message->headers->to = "Undisclosed-recipients";
     if(!message->headers->from)
       message->headers->from = mailfrom;
     if(!message->headers->subject)
       message->headers->subject = "";
     if(!message->headers->received)
       message->headers->received = received();
     else
       message->headers->received = received()
         + "\0"+message->headers->received;
     if(!message->headers["message-id"])
     {
       message->headers["message-id"] = sprintf("<%d@%s>", messageid,
						gethostname());
     }
     return message;
   }
   
   static MIME.Message low_message(string content)
   {
     datamode = 0;
     MIME.Message message;
     mixed err = catch (message = MIME.Message(content, 0, 0, 1));
     if(err)
     {
       outcode(554, err[0]);
       log(describe_backtrace(err));
       return 0;
     }
     err = catch {
       message = format_headers(message);
     };
     if(err)
     {
       outcode(554, err[0]);
       log(describe_backtrace(err));
       return 0;
     }
     return message;
   }

   void message(string content) {
     if(sizeof(content) > cfg->maxsize)
     {
        outcode(552);
        return;
     }
     MIME.Message message = low_message(content);
     if(!message) return;

     // SMTP mode, cb_data is called one time with an array of recipients
     // and the same MIME object
     int check;
     mixed err;
     if(cfg->givedata)
       err = catch(check = cfg->cb_data(message, mailfrom, mailto, content));
     else
       err = catch(check = cfg->cb_data(message, mailfrom, mailto));
     if(err)
     {
       outcode(554, err[0]);
       log(describe_backtrace(err));
       return;
     }
     outcode(getretcode(check), geterrorstring(check));
   }

   void noop()
   {
     remove_call_out(handle_timeout);
     call_out(handle_timeout, 310, "NOOP");
     outcode(250);
   }
  
   void rset()
   {
     remove_call_out(handle_timeout);
     call_out(handle_timeout, 310, "RSET");
     inputbuffer = "";
     mailfrom = "";
     mailto = ({ });
     messageid = 0;
     //sequence = ({ });
     outcode(250);
   }
   
   void vrfy()
   {
     remove_call_out(handle_timeout);
     call_out(handle_timeout, 310, "VRFY");
     outcode(252);
   }
   
   void quit()
   {
     fd->write("221 " + replace(replycodes[221], "<host>", localhost)
	       + "\r\n");
     shutdown_fd();
   }
   
   static int launch_functions(string line)
   {
     array(string) command = line / " ";
     // success
     if(sizeof(command) > 0)
     {
       string _command = lower_case(command[0]);
       mixed err = 0;
       if(has_index(commands, _command))
       {
         err = catch 
         {
#ifdef SMTP_DEBUG
            log("calling %O\n", _command);
#endif
#if constant(this)
	    function fun = this[_command]; // XXX: This line probably needs a patch!
#else
	    function fun = commands[_command];
#endif
	    fun(command[1..] * " ");
         };
       }
       else
       {
         log("command %O not recognized", _command);
         outcode(500);
       }
       if(err)
       {
         log("error while executing command %O", _command);
	 outcode(554, err[0]);
       }
     }
   }
  
   static void read_cb(mixed id, string data)
   {
     string pattern;
     int bufferposition;
     inputbuffer += data;
     int sizeofdata = sizeof(data);
     // optimization : don't search all the data, only the last one
     int searchpos = sizeof(inputbuffer) - sizeofpreviousdata-sizeofdata;
     sizeofpreviousdata = sizeofdata;
     if(searchpos < 0)
       searchpos = 0;
     datamode ? (pattern = "\r\n.\r\n"):(pattern = "\r\n");
     bufferposition = search(inputbuffer, pattern, searchpos);
     while(bufferposition != -1)
     {
#ifdef SMTP_DEBUG
       write("buffposition=%d, inputbuffer=%O\n", bufferposition, inputbuffer);
#endif
       bufferposition += sizeof(pattern);
       int end = bufferposition-(1+sizeof(pattern));
       if(!datamode)
       {
         launch_functions(inputbuffer[..end]);
         if(lower_case(inputbuffer[..end]) == "quit")
         {
#if constant(this)
           destruct(this);
#else
           destruct(this_object());
#endif
           return;
         }
         pattern = "\r\n";
       }
       if(datamode)
       {
         if(pattern=="\r\n.\r\n")
           message(inputbuffer[..end]);
         pattern = "\r\n.\r\n";
       }
       // end of buffer detection
       if(bufferposition + sizeof(pattern) >= sizeof(inputbuffer))
       {
#ifdef SMTP_DEBUG
         write("breaking\n");
#endif
         inputbuffer = "";
         break;
       }
       inputbuffer = inputbuffer[bufferposition..];
       bufferposition = search(inputbuffer, pattern);
     }
   }
   
   static void write_cb()
   {
     fd->write("220 " + replace(replycodes[220], "<host>", localhost)
	       + "\r\n");
     fd->set_write_callback(0);
   }

   static void shutdown_fd()
   {
     remove_call_out(handle_timeout);
#if constant(thread_create)
     object lock = Thread.Mutex()->lock();
#endif
     catch {
       fd->set_read_callback  (0);
       fd->set_write_callback (0);
       fd->set_close_callback (0);
       fd->close();
       destruct(fd);
     };
#if constant(thread_create)
     destruct(lock);
#endif
   }
   
   static void close_cb(int i_close_the_stream)
   {
     if(!i_close_the_stream)
     {
       string errmsg = "Connexion closed by client ";
       if(sequence && sizeof(sequence) > 1)
         errmsg += sequence[-1];
       log(errmsg);
     }
     shutdown_fd();
   }

   void create(object _fd, Configuration _cfg)
   {
     cfg = _cfg;
     features += cfg->get_features();

     fd->assign(_fd);
     catch(remoteaddr=((fd->query_address()||"")/" ")[0]);
     catch(localaddr=((fd->query_address(1)||"")/" ")[0]);
     catch(localport=(int)((fd->query_address(1)||"")/" ")[1]);
     if(!remoteaddr)
     {
       fd->write("421 " + replace(replycodes[421], "<host>", localhost)
		 + "\r\n");
       shutdown_fd();
       return;
     }
     if(!localaddr)
     {
       fd->write("421 " + replace(replycodes[421], "<host>", localhost)
		 + "\r\n");
       shutdown_fd();
       return;
     }
     //log("connection from %s to %s:%d", remoteaddr, localaddr, localport);
     fd->set_nonblocking(read_cb, write_cb, close_cb);
     call_out(handle_timeout, 300, "'First connexion'");
   }

   void destroy()
   {
     shutdown_fd();
   }

};

//! The use of Protocols.SMTP.server is quite easy and allow you 
//!  to design custom functions to process mail. This module does not
//!  handle mail storage nor relaying to other domains.
//! So it is your job to provide mail storage and relay mails to other servers
class Server {

   static object fdport;
   Configuration config;

   static void accept_callback()
   {
     object fd = fdport->accept();
     if(!fd)
       error("Can't accept connections from socket\n");
     Connection(fd, config);
     destruct(fd);
   }

   //! @decl void create(array(string) _domains, void|int port,@
   //! 				void|string ip, function _cb_mailfrom,@
   //!				function _cb_rcptto, function _cb_data)
   //!	Create a receiving SMTP server. It implements RFC 2821, 2822 and 1854.
   //! 
   //! @param domain
   //!   Domains name this server relay, you need to provide at least one
   //!   domain (the first one will be used for MAILER-DAEMON address).
   //!   if you want to relay everything you can put a '*' after this
   //!   first domain.
   //! @param port
   //!   Port this server listen on
   //! @param listenip
   //!   IP on which server listen
   //! @param cb_mailfrom
   //!   Mailfrom callback function, this function will be called
   //!   when a client send a mail from command. This function must take a
   //!   string as argument (corresponding to the sender's email) and return
   //!   int corresponding to the SMTP code to output to the client. If you
   //!   return an array the first element is the SMTP code and the second
   //!   is the error string to display.
   //! @param cb_rcptto
   //!   Same as cb_mailfrom but called when a client sends a rcpt to.
   //! @param cb_data
   //!  This function is called each time a client send a data content.
   //!  It must have the following synopsis:
   //!  int cb_data(object mime, string sender, array(string) recipients,@
   //!  void|string rawdata)
   //!  object mime : the mime data object
   //!  string sender : sender of the mail (from the mailfrom command)
   //!  array(string) recipients : one or more recipients given by the rcpt 
   //!     to command
   //! return : SMTP code to output to the client. If you return an array 
   //!   the first element is the SMTP code and the second is the error string
   //!   to display.
   //! @example
   //!  Here is an example of silly program that does nothing except outputting
   //!  informations to stdout.
   //! int cb_mailfrom(string mail)
   //! {
   //!   return 250;
   //! }
   //!
   //! int cb_rcptto(string email)
   //! {
   //!   // check the user's mailbox here
   //!   return 250;
   //! }
   //! 
   //! int cb_data(object mime, string sender, array(string) recipients)
   //! {
   //!   write(sprintf("smtpd: mailfrom=%s, to=%s, headers=%O\ndata=%s\n", 
   //!   sender, recipients * ", ", mime->headers, mime->getdata()));
   //!   // check the data and deliver the mail here
   //!   if(mime->body_parts)
   //!   {
   //!     foreach(mime->body_parts, object mpart)
   //!       write("smtpd: mpart data = %O\n", mpart->getdata());
   //!   }
   //!   return 250;
   //! }
   //! 
   //! int main(int argc, array(string) argv)
   //! {
   //!   Protocols.SMTP.Server(({ "ece.fr" }), 2500, "127.0.0.1", @
   //!      cb_mailfrom, cb_rcptto, cb_data);
   //!   return -1;
   //! }
   void create(array(string) _domains, void|int port, void|string ip, function _cb_mailfrom,
     function _cb_rcptto, function _cb_data)
   {
     config = Configuration(_domains, _cb_mailfrom, _cb_rcptto, _cb_data);
     random_seed(getpid() + time());
     if(!port)
       port = 25;
     fdport = Stdio.Port(port, accept_callback, ip);
     if(!fdport)
     {
       error("Cannot bind to socket, already bound ?\n");
     }
   }
};
