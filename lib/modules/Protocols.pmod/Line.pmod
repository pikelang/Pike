/*
 * $Id: Line.pmod,v 1.6 1998/10/16 00:08:11 nisse Exp $
 *
 * Line-buffered protocol handling.
 *
 * Henrik Grubbström 1998-05-27
 */

class simple
{
  static object con;

  function handle_data;
  void handle_command(string data);

  static int timeout;		// Idle time before timeout.
  static int timeout_time;	// Time at which next timeout will occur.

  static void send(string s)
  {
    send_q->put(s);
    con->set_write_callback(write_callback);
  }

  static void do_timeout()
  {
    if (con) {
      catch {
	con->close();
      };
      catch {
	// FIXME: Don't do this. It will break SSL connections. /nisse
	destruct(con);
      };
    }
  }

  static void _timeout_cb()
  {
    if (timeout > 0) {
      // Timeouts are enabled.

      int t = time();

      if (t >= timeout_time) {
	// Time out
	do_timeout();
      } else {
	// Not yet.
	call_out(_timeout_cb, timeout_time - t);
      }
    }
  }

  void touch_time()
  {
    if (timeout > 0) {
      timeout_time = time() + timeout;
    }
  }

  static string multi_line_buffer = "";
  static void _handle_command(string line)
  {
    if (handle_data) {
      if (line != ".") {
	multi_line_buffer += line + "\r\n";
      } else {
	function handle = handle_data;
	string data = multi_line_buffer;
	handle_data = 0;
	multi_line_buffer = "";
	handle(data);
      }
    } else {
      handle_command(line);
    }
  }

  static string read_buffer = "";

  static string get_line()
  {
    int i = search(read_buffer, "\r\n");
    if (i == -1) {
      return 0;
    }
    string data = read_buffer[..i-1];			// Not the "\r\n".
    read_buffer = read_buffer[i+2..];

    return data;
  }
  
  static void read_callback(mixed ignored, string data)
  {
    touch_time();

    read_buffer += data;

    string line;
    
    while( (line = get_line()) )
      _handle_command(line);
  }

  object(ADT.queue) send_q = ADT.queue();

  static string write_buffer = "";
  static void write_callback(mixed ignored)
  {
    touch_time();

    while (!sizeof(write_buffer)) {
      if (send_q->is_empty()) {
	con->set_write_callback(0);
	return;
      } else {
	write_buffer = send_q->get();
	if (!write_buffer) {
	  // EOF
	  con->set_write_callback(0);
	  con->close();
	  // FIXME: Don't do this! It will break SSL connections. /nisse
	  catch { destruct(con); };
	  con = 0;
	  return;
	}
      }
    }
    int w = con->write(write_buffer);
    if (w > 0) {
      write_buffer = write_buffer[w..];
      if (!sizeof(write_buffer)) {
	if (send_q->is_empty()) {
	  con->set_write_callback(0);
	} else {
	  write_buffer = send_q->get();
	  if (!write_buffer) {
	    // EOF
	    con->set_write_callback(0);
	    con->close();
	    catch { destruct(con); };
	    con = 0;
	  }
	}
      }
    } else {
      // Failed to write!
      werror("write_callback(): write() failed!\n");
		     
      con->set_write_callback(0);
      con->close();
      con = 0;
    }
  }

  void disconnect()
  {
    // Delayed disconnect.
    send_q->put(0);
    con->set_write_callback(write_callback);
    con->set_read_callback(0);
  }
    
  static void close_callback()
  {
    if (handle_data || sizeof(read_buffer) || sizeof(multi_line_buffer)) {
      werror("close_callback(): Unexpected close!\n");
    }
    con->close();
    con = 0;
  }

  void create(object con_, int|void timeout_)
  {
    con = con_;
    timeout = timeout_;

    // Start the timeout handler.
    touch_time();
    _timeout_cb();

    con->set_nonblocking(read_callback, 0, close_callback);
  }
};

class smtp_style
{
  inherit simple;

  constant errorcodes = ([]);

  void send(int code, array(string)|string|void lines)
  {
    lines = lines || errorcodes[code] || "Error";

    if (stringp(lines)) {
      lines /= "\n";
    }

    string init = sprintf("%03d", code);
    string res = "";

    int i;
    for(i=0; i < sizeof(lines)-1; i++) {
      res += init + "-" + lines[i] + "\r\n";
    }
    res += init + " " + lines[-1] + "\r\n";
    send_q->put(res);
    con->set_write_callback(write_callback);
  }
};

class imap_style
{
  inherit simple;

  function handle_literal = 0;
  int literal_length;

#if 0
  function timeout_handler = 0;
#endif
  
  static void read_callback(mixed ignored, string data)
  {
    touch_time();

    read_buffer += data;

    while(1) {
      if (handle_literal)
      {
	if (strlen(read_buffer) < literal_length)
	  return;
	string literal = read_buffer[..literal_length - 1];
	read_buffer = read_buffer[literal_length..];

	function handler = handle_literal;
	handle_literal = 0;

	handler(literal);
      } else {
	string line = get_line();
	if (line)
	  handle_command(line);
	else
	  break;
      }
    }
  }

  void expect_literal(int length, function callback)
  {
    literal_length = length;
    handle_literal = callback;
  }

#if 0
  static void do_timeout()
  {
    if (timeout_handler)
    {
      con->set_read_callback(0);
      con->set_close_callback(0);
      
      timeout_handler();
    } else 
      ::do_timeout();
  }

  void create(object con_, int|void timeout_, function|void callback)
  {
    timeout_handler = callback;
    ::create(con_, timeout_);
  }
#endif
}
