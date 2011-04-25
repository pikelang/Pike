/*
 * $Id$
 *
 * Line-buffered protocol handling.
 *
 * Henrik Grubbström 1998-05-27
 */

#pike __REAL_VERSION__

//! Simple nonblocking line-oriented I/O.
class simple
{
  static object con;

  //! The sequence separating lines from eachother. "\r\n" by default.
  static constant line_separator = "\r\n";

  //! If this variable has been set, multiple lines will be accumulated,
  //! until a line with a single @expr{"."@} (period) is received.
  //! @[handle_data()] will then be called with the accumulated data
  //! as the argument.
  //!
  //! @note
  //! @[handle_data()] is one-shot, ie it will be cleared when it is called.
  //!
  //! The line with the single @expr{"."@} (period) will not be in the
  //! accumulated data.
  //!
  //! @seealso
  //!   @[handle_command()]
  //!
  function(string:void) handle_data;

  //! This function will be called once for every line that is received.
  //!
  //! Overload this function as appropriate.
  //!
  //! @note
  //! It will not be called if @[handle_data()] has been set.
  //!
  //! @[line] will not contain the line separator.
  //!
  //! @seealso
  //! @[handle_data()]
  //!
  void handle_command(string line);

  static int timeout;		// Idle time before timeout.
  static int timeout_time;	// Time at which next timeout will occur.

  //! Queue some data to send.
  //!
  //! @seealso
  //! @[handle_command()], @[handle_data()], @[disconnect()]
  //!
  static void send(string s)
  {
    send_q->put(s);
    con->set_write_callback(write_callback);
  }

  //! This function is called when a timeout occurrs.
  //! 
  //! Overload this function as appropriate.
  //!
  //! The default action is to shut down the connection immediately.
  //!
  //! @seealso
  //! @[create()], @[touch_time()]
  //!
  static void do_timeout()
  {
    if (con) {
      catch {
	con->set_nonblocking(0,0,0);	// Make sure all callbacks are cleared.
      };
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

  //! Reset the timeout timer.
  //!
  //! @seealso
  //! @[create()], @[do_timeout()]
  //!
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
	multi_line_buffer += line + line_separator;
      } else {
	function(string:void) handle = handle_data;
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

  //! Read a line from the input.
  //!
  //! @returns
  //! Returns @expr{0@} when more input is needed.
  //! Returns the requested line otherwise.
  //!
  //! @note
  //! The returned line will not contain the line separator.
  //!
  //! @seealso
  //!   @[handle_command()], @[line_separator]
  //!
  static string read_line()
  {
    // FIXME: Should probably keep track of where the search ended last time.
    int i = search(read_buffer, line_separator);
    if (i == -1) {
      return 0;
    }
    string data = read_buffer[..i-1];		// Not the line separator.
    read_buffer = read_buffer[i+sizeof(line_separator)..];

    return data;
  }

  //! Called when data has been received.
  //!
  //! Overload as appropriate.
  //!
  //! Calls the handle callbacks repeatedly until no more lines are available.
  //!
  //! @seealso
  //!   @[handle_data()], @[handle_command()], @[read_line()]
  //!
  static void read_callback(mixed ignored, string data)
  {
    touch_time();

    read_buffer += data;

    string line;
    
    while( (line = read_line()) )
      _handle_command(line);
  }

  //! Queue of data that is pending to send.
  //!
  //! The elements in the queue are either strings with data to send,
  //! or @expr{0@} (zero) which is the end of file marker. The connection
  //! will be closed when the end of file marker is reached.
  //!
  //! @seealso
  //! @[send()], @[disconnect()]
  //!
  object(ADT.Queue) send_q = ADT.Queue();

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

  //! Disconnect the connection.
  //!
  //! Pushes an end of file marker onto the send queue @[send_q].
  //!
  //! @note
  //! Note that the actual closing of the connection is delayed
  //! until all data in the send queue has been sent.
  //!
  //! No more data will be read from the other end after this function
  //! has been called.
  //!
  void disconnect()
  {
    // Delayed disconnect.
    send_q->put(0);
    con->set_write_callback(write_callback);
    con->set_read_callback(0);
  }
    
  //! This function is called when the connection has been closed
  //! at the other end.
  //!
  //! Overload this function as appropriate.
  //!
  //! The default action is to shut down the connection on this side
  //! as well.
  //!
  static void close_callback()
  {
    if (handle_data || sizeof(read_buffer) || sizeof(multi_line_buffer)) {
      werror("close_callback(): Unexpected close!\n");
    }
    con->set_nonblocking(0,0,0);	// Make sure all callbacks are cleared.
    con->close();
    con = 0;
  }

  //! Create a simple nonblocking line-based protocol handler.
  //!
  //! @[con] is the connection.
  //!
  //! @[timeout] is an optional timeout in seconds after which the connection
  //! will be closed if there has been no data sent or received.
  //!
  //! If @[timeout] is @expr{0@} (zero), no timeout will be in effect.
  //!
  //! @seealso
  //! @[touch_time()], @[do_timeout()]
  //!
  void create(object(Stdio.File) con, int|void timeout)
  {
    this_program::con = con;
    this_program::timeout = timeout;

    // Start the timeout handler.
    touch_time();
    _timeout_cb();

    con->set_nonblocking(read_callback, 0, close_callback);
  }
};

//! Nonblocking line-oriented I/O with support for sending SMTP-style codes.
class smtp_style
{
  inherit simple;

  //! @decl mapping(int:string|array(string)) errorcodes
  //!
  //! Mapping from return-code to error-message.
  //!
  //! Overload this constant as apropriate.
  //!

  constant errorcodes = ([]);

  //! Send an SMTP-style return-code.
  //!
  //! @[code] is an SMTP-style return-code.
  //!
  //! If @[lines] is omitted, @[errorcodes] will be used to lookup
  //! an appropriate error-message.
  //!
  //! If @[lines] is a string, it will be split on @expr{"\n"@}
  //! (newline), and the error-code interspersed as appropriate.
  //!
  //! @seealso
  //! @[errorcodes]
  //!
  void send(int(100 .. 999) code, array(string)|string|void lines)
  {
    lines = lines || errorcodes[code] || "Error";

    if (stringp(lines)) {
      lines /= "\n";
    }

    string init = sprintf("%03d", code);
    string res = "";

    int i;
    for(i=0; i < sizeof(lines)-1; i++) {
      res += init + "-" + lines[i] + line_separator;
    }
    res += init + " " + lines[-1] + line_separator;
    send_q->put(res);
    con->set_write_callback(write_callback);
  }
};

//! Nonblocking line-oriented I/O with support for reading literals.
class imap_style
{
  inherit simple;

  //! Length in bytes of the literal to read.
  int literal_length;

  //! If this variable has been set, @[literal_length] bytes will be
  //! accumulated, and this function will be called with the resulting data.
  //!
  //! @note
  //! @[handle_literal()] is one-shot, ie it will be cleared when it is called.
  //!
  function(string:void) handle_literal;

  //! This function will be called once for every line that is received.
  //!
  //! @note
  //!   This API is provided for backward compatibility; overload
  //!   @[handle_command()] instead.
  //!
  //! @seealso
  //!   @[Protocols.Line.simple()->handle_command()]
  function(string:void) handle_line;

  //! Function called once for every received line.
  void handle_command(string line)
  {
    handle_line(line);
  }

  static void read_callback(mixed ignored, string data)
  {
    touch_time();

    read_buffer += data;

    while(1) {
      if (handle_literal)
      {
	if (sizeof(read_buffer) < literal_length)
	  return;
	string literal = read_buffer[..literal_length - 1];
	read_buffer = read_buffer[literal_length..];

	function handler = handle_literal;
	handle_literal = 0;

	handler(literal);
      } else {
	string line = read_line();
	if (line)
	  handle_command(line);
	else
	  break;
      }
    }
  }

  //! Enter literal reading mode.
  //!
  //! Sets @[literal_length] and @[handle_literal()].
  //!
  //! @seealso
  //! @[literal_length], @[handle_literal()]
  //!
  void expect_literal(int length, function(string:void) callback)
  {
    literal_length = length;
    handle_literal = callback;
  }
}
