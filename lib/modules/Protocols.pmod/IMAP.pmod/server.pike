/* IMAP.server
 *
 * Handles the server side of the protocol.
 */

#pike __REAL_VERSION__

inherit Protocols.Line.imap_style;

int debug_level;

void send_line(string s)
{
  if (debug_level) {
    werror(sprintf("IMAP.server::send_line(%O)\n", s));
  }
  send(s + "\r\n");
}

void send_lines(string ... s)
{
  if (debug_level) {
    werror(sprintf("IMAP.server::send_lines(%O)\n", s));
  }
  send_line(s * "\r\n");
}

void close_imap()
{
  disconnect();
}

void do_timeout()
{
  if (con)
  {
    send_line("* BYE Timeout");
    disconnect();
  }
}

mapping(string:function) commands;

function(object:void) request_callback;

void recv_command(string s)
{
  if (!sizeof(s))
    // Ignore empty lines.
    return;
  
  object line = .parse_line(s);

  // trace(4711);

  string tag = line->get_atom();

  if (debug_level)
    werror("Read tag: %O\n", tag);
  
  if (!tag)
  {
    // werror("Foo!\n");
    send_bad_response(tag, "No tag");
    return;
  }
  
  int state;
  int|function req;

  do {
    string command = line->get_atom();

    if (!command)
    {
      send_bad_response(tag, "No command");
      return;
    }

    if (debug_level)
      werror("Read command: %O\n", command);
    
    req = commands[lower_case(command)];
    if (intp(req)) {
      if (!req || state)
      {
	send_bad_response(tag, upper_case(command)+" Unknown command");	
	return;
      }
      state = req;
    } else {
      // Found a function.
      break;
    }
  } while (1);

  request_callback(req(tag, line, state));
}

class recv_line
{
  function handler;

  void create(function h)
    {
      handler = h;
    }

  void `()(string s) { handler(.parse_line(s)); }
}

class recv_literal
{
  function handler;
  
  void create(function h)
    {
      handler = h;
    }

  void `()(string s)
    {
      handler(s);
    }
}


/* Functions that can usefully be called by the request handler */

/* NOTE: This function sends an \r\n-pair even if the last argument is
 * sent as a literal. I don't understand whether or not this behaviour
 * is correct according to rfc-2060.*/
void send_imap(string|object ...args)
{
  send_line(.types.imap_format_array(args));
}

void send_bad_response(string tag, string msg)
{
  tag = tag || "*";
  send_imap(tag, "BAD", msg);
}

void send_ok_response(string tag, string msg)
{
  send_imap(tag, "OK", msg);
}

void send_continuation_response(string msg)
{
  send_imap("+", msg);
}

void use_commands(mapping(string:function) c)
{
  commands = c;
}

void get_request()
{
  handle_line = recv_command;
}

void get_line(function handler)
{
  handle_line = recv_line(handler);
}

void get_literal(int length, function handler)
{
  literal_length = length;
  handle_literal = recv_literal(handler);
}

void create(object f, int timeout,
	    function(object:void) callback, int|void debug)
{
  ::create(f, timeout);
  debug_level = debug;
  request_callback = callback;
  
  get_request();
}
