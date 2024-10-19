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
   454:"TLS not available due to temporary reason",
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

class AsyncProtocol
{
  inherit Protocols.NNTP.asyncprotocol;
}

//! A helper class with functions useful when sending eMail.
class ClientHelper
{

  //! Parses email addresses as the @[Protocols.SMTP] client classes do.
  //! Useful if emails should only be sent matching certain conditions etc..
  protected string parse_addr(string addr)
  {
    array(string|int) tokens = replace(MIME.tokenize(addr), '@', "@");

    int i;
    tokens = tokens[search(tokens, '<') + 1..];

    if ((i = search(tokens, '>')) != -1) {
      tokens = tokens[..i-1];
    }
    return tokens*"";
  }

  //! Return an @rfc{2822@} date-time string suitable for the
  //! @tt{Date:@} header.
  string rfc2822date_time(int ts)
  {
    mapping(string:int) lt = localtime(ts);
    int zsgn = sgn(lt->timezone);
    int zmin = (zsgn * lt->timezone)/60;
    int zhr = zmin/60;
    zmin -= zhr*60;
    int zone = -zsgn*(zhr*100 + zmin);
    return sprintf("%s, %02d %s %04d %02d:%02d:%02d %+05d",
		   ({ "Sun", "Mon", "Tue", "Wed", "Thu",
		      "Fri", "Sat" })[lt->wday], lt->mday,
		   ({ "Jan", "Feb", "Mar", "Apr",
		      "May", "Jun", "Jul", "Aug",
		      "Sep", "Oct", "Nov", "Dec" })[lt->mon],
		   lt->year + 1900,
		   lt->hour, lt->min, lt->sec,
		   zone);
  }
}

//! Synchronous (blocking) email class (this lets you send emails).
class Client
{
  inherit Protocol;
  inherit ClientHelper;

  int errorcode = 0;
  protected int cmd(string c, string|void comment)
  {
    int r = errorcode = command(c);
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
  //!
  //! Creates an SMTP mail client and connects it to the the @[server]
  //! provided. The server parameter may either be a string with the
  //! hostname of the mail server, or it may be a file object acting
  //! as a mail server.  If @[server] is a string, then an optional
  //! port parameter may be provided. If no port parameter is
  //! provided, port 25 is assumed. If no parameters at all is
  //! provided the client will look up the mail host by searching for
  //! the DNS MX record.
  //!
  //! @throws
  //!   Throws an exception if the client fails to connect to
  //!   the mail server.
  protected void create(void|string|Stdio.File server, int|void port)
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

  protected string punycode_address(string address)
  {
    // FIXME: Add support RFC5336 and the UTF8SMTP EHLO-extension.
    catch { address = utf8_to_string(address); };
    array(string|int) tokens =
      MIME.decode_words_tokenized_remapped(address);
    int in_hostname;
    foreach(tokens; int i; string|int token) {
      if (token == '@') in_hostname = 1;
      if (!stringp(token)) continue;
      if (in_hostname) {
	token = map(token/".", Standards.IDNA.to_ascii) * ".";
      } else {
	token = string_to_utf8(token);
      }
      tokens[i] = token;
    }

    return MIME.quote(tokens);
  }

  //! Sends a mail message from @[from] to the mail addresses listed
  //! in @[to] with the mail body @[body]. The body should be a
  //! correctly formatted mail DATA block, e.g.  produced by
  //! @[MIME.Message].
  //!
  //! @seealso
  //!   @[simple_mail]
  //!
  //! @throws
  //!   If the mail server returns any other return code than
  //!   200-399 an exception will be thrown.
  void send_message(string from, array(string) to, string body)
  {
    cmd("MAIL FROM: <" + punycode_address(from) + ">");
    foreach(to, string t) {
      cmd("RCPT TO: <" + punycode_address(t) + ">");
    }
    cmd("DATA");

    // Perform quoting according to RFC 2821 4.5.2.
    // and 2.3.7
    if (sizeof(body) && body[0] == '.') {
      body = "." + body;
    }
    body = replace(body, ({
		     "\r\n.",
		     "\r\n",
		     "\r",
		     "\n",
		   }),
		   ({
		     "\r\n..",
		     "\r\n",
		     "\r\n",
		     "\r\n",
		   }));

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

  //! Sends an e-mail. Wrapper function that uses @[send_message].
  //!
  //! @note
  //!   Some important headers are set to:
  //!   @expr{"Content-Type: text/plain; charset=iso-8859-1"@} and
  //!   @expr{"Content-Transfer-Encoding: 8bit"@}.
  //!   The @expr{"Date:"@} header is set to the current local time.
  //!   The @expr{"Message-Id"@} header is set to a @[Standards.UUID]
  //!   followed by the hostname as returned by @[gethostname()].
  //!
  //! @note
  //!   If @[gethostname()] is not supported, it will be replaced
  //!   with the string @expr{"localhost"@}.
  //!
  //! @throws
  //!   If the mail server returns any other return code than
  //!   200-399 an exception will be thrown.
  void simple_mail(string to, string subject, string from, string msg)
  {
    if (!has_value(msg, "\r\n"))
      msg=replace(msg,"\n","\r\n"); // *simple* mail /Mirar
    string msgid = "<" + (string)Standards.UUID.make_version1(-1) + "@" +
#if constant(gethostname)
      gethostname() +
#else
      "localhost"
#endif
      ">";
    send_message(parse_addr(from), ({ parse_addr(to) }),
		 (string)MIME.Message(msg, (["mime-version":"1.0",
					     "subject":subject,
					     "from":from,
					     "to":to,
					     "content-type":
					       "text/plain;charset=iso-8859-1",
					     "content-transfer-encoding":
					       "8bit",
					     "date":rfc2822date_time(time(1)),
					     "message-id": msgid,
				      ])));
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
  //!   verification queries in order to prevent spammers
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

//! Asynchronous (nonblocking/event-oriented) email client class (this lets
//! you send emails).
class AsyncClient
{
  inherit AsyncProtocol;
  inherit ClientHelper;

  int errorcode = 0;
  int(0..1) no_error, is_started, in_sequence;
  string error_string;
  function error_cb;
  array error_args;
  function(int,string:void) cb;
  array sequences = ({ }), invokes = ({ });
  mapping error_cbs = ([ ]);
  string|object server;
  int port;
  array args;

  void invoke(function f, mixed ... args)
  {
    if(!f) error("No function supplied.\n");
    invokes += ({ f, args });
    call_out(invoke_next, 0);
  }

  void invoke_next() {
    array a = invokes[..1];

    invokes = invokes[2..];
    a[0](@a[1]);
  }

  void my_error(string fmt, mixed ... args)
  {
    error_string = sprintf(fmt, @args);
    if(!no_error && error_cb) invoke(error_cb,
				     ({ 0, errorcode, error_string, 1 }),
				     @error_args);
    if(!no_error) foreach(error_cbs; function f; array args2)
      foreach(args2;; array args3)
	invoke(f, ({ 0, errorcode, error_string, 0 }), @args3);
  }

  void io_error(string fmt, mixed ... args) {
    error_string = sprintf(fmt, @args);
    if(error_cb) invoke(error_cb, ({ 1, errno(), error_string, 1 }),
			@error_args);
    foreach(error_cbs; function f; array args2)
      foreach(args2;; array args3)
	invoke(f, ({ 1, errno(), error_string, 0 }), @args3);
  }

  protected void cmd(string c, string|void comment, function|void cb,
		     mixed ... extra)
  {
    error_string = 0;
    command(c, lambda(int i) {
      int r = errorcode = i;

      switch(r) {
      case 200..399:
	break;
      default:
	error_string = "SMTP: "+c+"\n"+(comment?"SMTP: "+comment+"\n":"")+
	       "SMTP: "+replycodes[r]+"\n";
	if(!no_error && cb)
	  cb(({ r, error_string }), @extra);
	return;
      }

      if(cb) cb(r, @extra);
    });
  }

  void add_sequence(function cb, function error_cb, array err_args,
		    mixed ... args)
  {
    sequences += ({ cb, args, error_cb, err_args });
    if(error_cb)
    {
      if(!error_cbs[error_cb]) error_cbs[error_cb] = ({ });
      error_cbs[error_cb] += ({ err_args });
    }

    if(is_started && sizeof(sequences) == 4)
    {
      trigger_sequence();
    }
  }

  void trigger_sequence()
  {
    if(sizeof(sequences) && !in_sequence)
    {
      int(0..1) found;
      in_sequence = 1;
      no_error = 0;
      if(error_cbs[sequences[2]])
      {
	filter(error_cbs[sequences[2]], lambda(mixed a)
	       {
		 if(found) return found;
		 found = `==(a, sequences[3]);
		 return !found;
	       });
	if(!sizeof(error_cbs[sequences[2]])) m_delete(error_cbs, sequences[2]);
      }
      error_cb = sequences[2];
      error_args = sequences[3];
      sequences[0](@sequences[1]);
    }
  }

  void remove_sequence()
  {
    sequences = sequences[4..];
    in_sequence = 0;
    error_args = error_cb = 0;
    trigger_sequence();
  }

  function get_function(string name) {
    switch (name) {
      case "cmd":
	return cmd;
      default:
	return `->(this, name);
    }
  }

  function substitute_callback(function f, object server) {
      if (function_object(f) == server) {
	function oldf = f;
	f = get_function(function_name(f));
	if (!f) error("LOST A FUNCTION: %O, %O!\n", function_name(oldf), `->(this, function_name(oldf)));
      }

      return f;
  }

  mixed substitute_callback_rec(mixed m, object server) {
    if (functionp(m)) {
      return substitute_callback(m, server);
    } else if (arrayp(m))
    {
      array res = allocate(sizeof(m));

      foreach (m; int i; mixed m2) {
	res[i] = substitute_callback_rec(m2, server);
      }

      return res;
    } else if (mappingp(m))
    {
#if 1
      array k, v;
      k = indices(m);
      v = values(m);
      k = substitute_callback_rec(k, server);
      v = substitute_callback_rec(v, server);
      return mkmapping(k, v);
#else
      mapping new = ([ ]);
      foreach (indices(m);; mixed k) {
	  new[substitute_callback_rec(k, server)] = substitute_callback_rec(m[k], server);
      }
      return new;
#endif
    } else
    {
      return m;
    }
  }

  //! Creates an SMTP mail client and connects it to the the @[server]
  //! provided. The server parameter may either be a string with the
  //! hostname of the mail server, or it may be a file object acting
  //! as a mail server.  If @[server] is a string, then an optional
  //! port parameter may be provided. If no port parameter is
  //! provided, port 25 is assumed. If no parameters at all is
  //! provided the client will look up the mail host by searching for
  //! the DNS MX record.
  //!
  //! The callback will first be called when the connection is
  //! established (@expr{cb(1, @@args)@}) or fails to be established
  //! with an error. The callback will also be called with an error if
  //! one occurs during the delivery of mails.
  //!
  //! In the error cases, the @expr{cb@} gets called the following ways:
  //! @ul
  //! @item
  //! 	@expr{cb(({ 1, errno(), error_string, int(0..1) direct }), @@args);@}
  //! @item
  //! 	@expr{cb(({ 0, smtp-errorcode, error_string, int(0..1) direct }), @@args);@}
  //! @endul
  //! Where @expr{direct@} is @expr{1@} if establishment of the connection
  //! failed.
  protected void create(void|string|object server, int|void port,
			function|void cb, mixed ... args)
  {
    object dns=master()->resolv("Protocols")["DNS"];

    if (objectp(server)) {
      if (Program.inherits(object_program(server), Protocols.SMTP.AsyncClient)) {
	if (!port) port = server->port;
	if (!cb) cb = server->cb;
	if (!args) args = server->args;
	object t = server->server;
	this::port = port;
	this::args = args;
	this::cb = cb;

	// fetch queue
	foreach(server->error_cbs; function f; array args)
	{
	  error_cbs[substitute_callback(f, server)] = args;
	}
	error_cb = server->error_cb && substitute_callback(server->error_cb, server);
	error_args = server->error_args;
	//sequences = map(server->sequences, substitute_callback, server);
	sequences = substitute_callback_rec(server->sequences, server);

	destruct(server);
	this::server = server = t;
	initiate_connection(server, port, cb, @args);
	return;
      }
    }

    this::server = server;
    this::port = port;
    this::cb = cb;
    this::args = args;

    if(!error_cbs[cb]) error_cbs[cb] = ({ });
    error_cbs[cb] += ({ args });

    if(!server)
    {
      // Lookup MX record here (Using DNS.pmod)
      dns->async_get_mx(gethostname(), lambda(array a)
        {
          if (a)
          {
            dns->async_host_to_ip(a[0], lambda(string host, string ip)
              {
                initiate_connection(ip, port, cb, @args);
              });
          } else
            error("Failed to connect to mail server.\n");
        });
    } else
    {
       if (is_ip(server))
       {
	  initiate_connection(server, port, cb, @args);
       }
       else
	  dns->async_host_to_ip(server, lambda(string host, string ip)
            {
              initiate_connection(ip, port, cb, @args);
            });
    }
  }

  void initiate_connection(Stdio.File|string server, int|void port,
			   function|void cb, mixed ... args)
  {
    if (objectp(server)) {
       assign(server);
       established_connection(1, @args);
    } else
    {
       if(!port)
	  port = 25;

       if(!server || !async_connect(server, port, established_connection, cb,
				    @args))
       {
	 error("Failed to connect to mail server. (>>%O<<)\n", server);
       }
    }
  }

  void established_connection(int(0..1) success,
			      function|void cb, mixed ... args)
  {
    if (!success)
    {
      error("Failed to connect to mail server.\n");
      return;
    }

    readreturncode(issue_ehlo, cb, @args);
  }

  void issue_ehlo(int code, function|void cb, mixed ... args)
  {
    if(code/100 != 2)
    {
      error("Connection refused by SMTP server.\n");
      return;
    }

    no_error = 1;
    cmd("EHLO "+gethostname(), 0, issue_helo, cb, @args);
  }

  void issue_helo(int|array code, function|void cb, mixed ... args)
  {
    no_error = 0;

    if (error_string)
    {
      cmd("HELO "+gethostname(), "greeting failed.", connection_done, cb,
	  @args);
    } else
    {
      connection_done(1, cb, @args);
    }
  }

  void connection_done(array|int code, function|void cb, mixed ... args)
  {
    is_started = 1;
    call_out(trigger_sequence, 0);
    error_args = error_cb = 0;
    if(cb) cb(code, @args);
  }

  //! Sends a mail message from @[from] to the mail addresses
  //! listed in @[to] with the mail body @[body]. The body
  //! should be a correctly formatted mail DATA block, e.g.
  //! produced by @[MIME.Message].
  //!
  //! When the message is successfully sent, the callback will be called
  //! (@expr{cb(1, @@args);@}).
  //!
  //! When the message cannot be sent, @expr{cb@} will be called in one of the
  //! following ways:
  //! @ul
  //! @item
  //! 	@expr{cb(({ 1, errno(), error_string, int(0..1) direct }), @@args);@}
  //! @item
  //! 	@expr{cb(({ 0, smtp-errorcode, error_string, int(0..1) direct }), @@args);@}
  //! @endul
  //! where direct will be @expr{1@} if this particular message caused the
  //! error and @expr{0@} otherwise.
  //!
  //! @seealso
  //!   @[simple_mail]
  void send_message(string from, array(string) to, string body,
		    function|void cb, mixed ... args)
  {
    add_sequence(cmd, cb, args, "MAIL FROM: <" + from + ">", 0, send_message2,
		 to, 0, body, cb, @args);
  }

  void send_message2(int succ, array(string) to, int pos, string body,
		     function|void cb, mixed ... args)
  {
    if(pos < sizeof(to))
    {
      cmd("RCPT TO: <" + to[pos] + ">", 0, send_message2, to, pos+1, body, cb, @args);
      return;
    }

    cmd("DATA", 0, send_message3, body, cb, @args);
  }

  void send_message3(int succ, string body, function|void cb, mixed ... args) {
    // Perform quoting according to RFC 2821 4.5.2.
    // and 2.3.7
    if (sizeof(body) && body[0] == '.') {
      body = "." + body;
    }
    body = replace(body, ({
		     "\r\n.",
		     "\r\n",
		     "\r",
		     "\n",
		   }),
		   ({
		     "\r\n..",
		     "\r\n",
		     "\r\n",
		     "\r\n",
		   }));

    // RFC 2821 4.1.1.4:
    //   An extra <CRLF> MUST NOT be added, as that would cause an empty
    //   line to be added to the message.
    if (has_suffix(body, "\r\n"))
      body += ".";
    else
      body += "\r\n.";

    cmd(body, 0, send_message_done, cb, @args);
  }

  void send_message_done(int|array succ, function|void cb, mixed ... args)
  {
    remove_sequence();

    if(cb)
      cb(intp(succ), @args);
  }

  //! Sends an e-mail. Wrapper function that uses @[send_message].
  //!
  //! @note
  //!   Some important headers are set to:
  //!   @expr{"Content-Type: text/plain; charset=iso-8859-1"@} and
  //!   @expr{"Content-Transfer-Encoding: 8bit"@}. @expr{"Date:"@}
  //!   header isn't used at all.
  //!
  //! When the message is successfully sent, the callback will be called
  //! (@expr{cb(1, @@args);@}).
  //!
  //! When the message cannot be sent, @expr{cb@} will be called in one of the
  //! following ways:
  //! @ul
  //! @item
  //! 	@expr{cb(({ 1, errno(), error_string, int(0..1) direct }), @@args);@}
  //! @item
  //! 	@expr{cb(({ 0, smtp-errorcode, error_string, int(0..1) direct }), @@args);@}
  //! @endul
  //! where direct will be @expr{1@} if this particular message caused the
  //! error and @expr{0@} otherwise.
  void simple_mail(string to, string subject, string from, string msg,
		   function|void cb, mixed ... args)
  {
    if (!has_value(msg, "\r\n"))
      msg=replace(msg,"\n","\r\n"); // *simple* mail /Mirar
    string msgid = "<" + (string)Standards.UUID.make_version1(-1) + "@" +
#if constant(gethostname)
      gethostname() +
#else
      "localhost"
#endif
      ">";
    send_message(parse_addr(from), ({ parse_addr(to) }),
		 (string)MIME.Message(msg, (["mime-version":"1.0",
					     "subject":subject,
					     "from":from,
					     "to":to,
					     "content-type":
					       "text/plain;charset=iso-8859-1",
					     "content-transfer-encoding":
					       "8bit",
					     "date":rfc2822date_time(time(1)),
					     "message-id": msgid,
					    ])), cb, @args);
  }

  //! Verifies the mail address @[addr] against the mail server.
  //!
  //! The callback will be called with
  //! @ul
  //! @item
  //! 	@expr{cb(({ code, message }), @@args);@}
  //! @endul
  //! where code and message are
  //!  @array
  //!     @elem int code
  //!       The numerical return code from the VRFY call.
  //!     @elem string message
  //!       The textual answer to the VRFY call.
  //!  @endarray
  //! or
  //! @ul
  //! @item
  //! 	@expr{cb(({ 1, errno(), error_string, int(0..1) direct }), @@args);@}
  //! @item
  //! 	@expr{cb(({ 0, smtp-errorcode, error_string, int(0..1) direct }), @@args);@}
  //! @endul
  //! with @expr{direct@} being @expr{1@} if this verify operation caused the
  //! error when the message can't be verified
  //! or when an error occurs.
  //!
  //! @note
  //!   Some mail servers does not answer truthfully to
  //!   verification queries in order to prevent spammers
  //!   and others to gain information about the mail
  //!   addresses present on the mail server.
  void verify(string addr, function cb, mixed ... args)
  {
#if 0
    void _f(int|array result) {
      if(intp(result)) result = ({ result, rest });
      remove_sequence();
      cb(result, @args);
    };
#endif

    add_sequence(cmd, cb, args, "VRFY "+addr, 0, verify_cb, cb, @args);
  }

  void verify_cb(int|array result, function cb, mixed ... args) {
    if (intp(result)) result = ({ result, rest });
    remove_sequence();
    cb(result, @args);
  }
}

//! Class to store configuration variable for the SMTP server
class Configuration {

  //! Message max size
  int maxsize = 100240000;

  //! Maximum number of recipients (default 1000)
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

  protected void create(array(string) _domains, void|function _cb_mailfrom,
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

  protected Configuration cfg;

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
  constant internal_error_name = ". Problem due to the internal error: ";

  // the fd of the socket
  protected Stdio.File fd = Stdio.File();
  // the input buffer for read_cb
  protected string inputbuffer = "";
  // the size of the old data string in read_cb
  protected int sizeofpreviousdata = 0;
  // the from address
  protected string mailfrom = "";
  // to the address(es)
  protected array(string) mailto = ({ });
  // the ident we get from ehlo/helo
  protected string ident = "";
  // these are obvious
  protected string remoteaddr, localaddr;
  protected int localport;
  // my name
  protected string localhost = gethostname();

  // the sequence of commands the client send
  protected array(string) sequence = ({ });
  // the message id of the current mail
  protected string|int messageid;

  // whether you are in data mode or not...
  int datamode = 0;

  // the features this module support (fetched from Configuration - get_features()
   private array(string) features = ({ });

   protected void handle_timeout(string cmd)
   {
     string errmsg = "421 Error: timeout exceeded after command " +
       cmd || "unknown command!" + "\r\n";
     catch(fd->write(errmsg));
     log(errmsg);
     shutdown_fd();
   }

   // return true if the given return code from the call back function
   // is a success one or not
   protected int is_success(array|int check)
   {
     return (getretcode(check)/100 == 2);
   }

   // get the return code from the callback function
   protected int getretcode(array|int check)
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
   protected int|string geterrorstring(array|int check)
   {
     if(arrayp(check) && stringp(check[1]))
       return check[1];
     return 0;
   }

   protected void outcode(int code, void|string internal_error)
   {
     string msg = (string) code + " ";
     if(internal_error)
       msg += internal_error;
     else
       msg += replycodes[code];
     msg += "\r\n";
     fd->write(msg);
#ifdef SMTP_DEBUG
     log(msg);
#endif
   }

  //! This function is called whenever the SMTP server logs something.
  //! By default the log function is @[werror].
   function(string:mixed) logfunction = werror;

   protected void log(string fmt, mixed ... args)
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
   protected string received()
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

   // fetch the email address from the mail from: or rcpt to: commands
   // content: the input line like mail from:<toto@caudium.net>
   // what: the action either from or to
   int|string parse_email(string content, string what)
   {
      array parts = content / ":";
      if(lower_case(parts[0]) != what)
        return 500;
      string validating_mail;
      parts[1] = String.trim(parts[1]);
      if(!sscanf(parts[1], "<%s>", validating_mail))
        sscanf(parts[1], "%s", validating_mail);

      // Since we relay, we must accept mail for <postmaster> according to RFC 2821.
      // If we get mail for postmaster, we rewrite the address to postmaster@mydomain.
      if (validating_mail == "postmaster")
	validating_mail += "@" + cfg->domains[0];
      if(validating_mail == "")
        validating_mail = "MAILER-DAEMON@" + cfg->domains[0];
      array emailparts = validating_mail / "@";
      array(string) hostparts = lower_case(emailparts[1]) / ".";
      array(string) domains = ({});
      for(int i = sizeof(hostparts); i >= 2; i--)
      {
        domains += ({ (hostparts[sizeof(hostparts) - i ..] * ".") });
      }

      if(cfg->checkemail && sizeof(emailparts) != 2)
      {
        log("invalid mail address '%O', command=%O\n", emailparts, what);
        return 553;
      }
      if(cfg->checkdns)
      {
        if(what == "from")
        {
          int dnsok = 0;
          foreach(domains;; string domain)
          {
            if(Protocols.DNS.client()->get_primary_mx(domain))
              dnsok = 1;
          }
          if(!dnsok)
          {
           log("check dns failed, command=%O, domain=%O\n", what, domains);
            return 553;
          }
        }
      }
      if(what == "to")
      {
        int relayok = 0;
        foreach(domains;; string domain)
        {
          if(has_value(cfg->domains, domain))
            relayok = 1;
        }
        if(!relayok && !has_value(cfg->domains, "*"))
        {
          log("relaying denied, command=%O, cfg->domains=%O, domains=%O\n",
	      what, cfg->domains, domains);
          return 553;
        }
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
           outcode(451, internal_error_name + err[0]);
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
       // Mail directed to postmaster must be accepted according to RFC 2821.
       if (!((email/"@")[0]=="postmaster"))
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

   protected object(MIME.Message)|zero low_message(string content)
   {
     datamode = 0;
     MIME.Message message;
     mixed err = catch (message = MIME.Message(content, 0, 0, 1));
     if(err)
     {
       outcode(554, internal_error_name + err[0]);
       log(describe_backtrace(err));
       return 0;
     }
     err = catch {
       message = format_headers(message);
     };
     if(err)
     {
       outcode(554, internal_error_name + err[0]);
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
       outcode(554, internal_error_name + err[0]);
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

   protected int launch_functions(string line)
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
	    function fun = commands[_command];
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
	 outcode(554, internal_error_name + err[0]);
       }
     }
   }

   protected void read_cb(mixed id, string data)
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
           destruct(this);
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

   protected void write_cb()
   {
     fd->write("220 " + replace(replycodes[220], "<host>", localhost)
	       + "\r\n");
     fd->set_write_callback(0);
   }

   protected void shutdown_fd()
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

   protected void close_cb(int i_close_the_stream)
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

   protected void create(object _fd, Configuration _cfg)
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

   protected void _destruct()
   {
     shutdown_fd();
   }

};

//! The use of Protocols.SMTP.server is quite easy and allow you
//!  to design custom functions to process mail. This module does not
//!  handle mail storage nor relaying to other domains.
//! So it is your job to provide mail storage and relay mails to other servers
class Server {

   protected object fdport;
   Configuration config;

   protected void accept_callback()
   {
     object fd = fdport->accept();
     if(!fd)
       error("Can't accept connections from socket\n");
     Connection(fd, config);
     destruct(fd);
   }

   //!	Create a receiving SMTP server. It implements @rfc{2821@},
   //!	@rfc{2822@} and @rfc{1854@}.
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
   //!  Here is an example of silly program that does nothing except outputing
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
   protected void create(array(string) domains, void|int port, void|string ip,
			 function cb_mailfrom, function cb_rcptto,
			 function cb_data)
   {
     config = Configuration(domains, cb_mailfrom, cb_rcptto, cb_data);
     if(!port)
       port = 25;
     fdport = Stdio.Port(port, accept_callback, ip);
     if(!fdport)
     {
       error("Cannot bind to socket, already bound ?\n");
     }
   }

   protected void _destruct()
   {
     catch {
       fdport->close();
       destruct(fdport);
     };
   }

};
