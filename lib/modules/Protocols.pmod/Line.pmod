/*
 * $Id: Line.pmod,v 1.1 1998/05/27 20:56:39 grubba Exp $
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
  static void read_callback(mixed ignored, string data)
  {
    read_buffer += data;

    while(1) {
      int i = search(read_buffer, "\r\n");
      if (i == -1) {
	return;
      }
      data = read_buffer[..i-1];			// Not the "\r\n".
      read_buffer = read_buffer[i+2..];
      _handle_command(data);

      if (read_buffer == "") {
	return;
      }
    }
  }

  object(ADT.queue) send_q = ADT.queue();

  static string write_buffer = "";
  static void write_callback(mixed ignored)
  {
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

  void create(object con_)
  {
    con = con_;
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
