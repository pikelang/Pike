//
// $Id: TELNET.pmod,v 1.2 1998/04/05 19:05:44 grubba Exp $
//
// The TELNET protocol as described by RFC 764 and others.
//
// Henrik Grubbström <grubba@idonex.se> 1998-04-04
//

#ifdef TELNET_DEBUG
#define DWRITE(X)	werror(X)
#else
#define DWRITE(X)
#endif /* TELNET_DEBUG */

//. o protocol
//.   Implementation of the TELNET protocol.
class protocol
{
  //. + fd
  //.   The connection.
  static object fd;

  //. + cb
  //.   Mapping containing extra callbacks.
  static private mapping cb;

  //. + id
  //.   Value to send to the callbacks.
  static private mixed id;

  //. + write_cb
  //.   Write callback.
  static private function(mixed|void:string) write_cb;

  //. + read_cb
  //.   Read callback.
  static private function(mixed, string:void) read_cb;

  //. + close_cb
  //.   Close callback.
  static private function(mixed|void:void) close_cb;

  //. + TelnetCodes
  //.   Mapping of telnet codes to their abreviations.
  static private constant TelnetCodes = ([
    236:"EOF",		// End Of File
    237:"SUSP",		// Suspend Process
    238:"ABORT",	// Abort Process
    239:"EOR",		// End Of Record

    // The following ones are specified in RFC 959:
    240:"SE",		// Subnegotiation End
    241:"NOP",		// No Operation
    242:"DM",		// Data Mark
    243:"BRK",		// Break
    244:"IP",		// Interrupt Process
    245:"AO",		// Abort Output
    246:"AYT",		// Are You There
    247:"EC",		// Erase Character
    248:"EL",		// Erase Line
    249:"GA",		// Go Ahead
    250:"SB",		// Subnegotiation
    251:"WILL",		// Desire to begin/confirmation of option
    252:"WON'T",	// Refusal to begin/continue option
    253:"DO",		// Request to begin/confirmation of option
    254:"DON'T",	// Demand/confirmation of stop of option
    255:"IAC",		// Interpret As Command
  ]);

  //. + to_send
  //.   Data queued to be sent.
  static private string to_send = "";

  //. - write
  //.   Queues data to be sent to the other end of the connection.
  //. > s - String to send.
  void write(string s)
  {
    to_send += replace(s, "\377", "\377\377");
  }

  //. + write_raw
  //.   Queues raw data to be sent to the other end of the connection.
  //. > s - String with raw telnet data to send.
  void write_raw(string s)
  {
    to_send += s;
  }

  //. - send_data
  //.   Callback called when it is clear to send data over the connection.
  //.   This function does the actual sending.
  static private void send_data()
  {
    if (!sizeof(to_send)) {
      to_send = write_cb(id);
    }
    if (!to_send) {
      // Support for delayed close.
      fd->close();
      fd = 0;
    } else if (sizeof(to_send)) {
      if (to_send[0] == 242) {
	// DataMark needs extra quoting... Stupid.
	to_send = "\377\361" + to_send;
      }

      int n = fd->write(to_send);

      to_send = to_send[n..];
    }
  }

  //. + default_cb
  //.   Mapping with the default handlers for TELNET commands.
  static private mapping(string:function) default_cb = ([
    "BRK":lambda() {
	    destruct();
	    throw(0);
	  },
    "AYT":lambda() {
	    to_send += "\377\361";	// NOP
	  },
    "WILL":lambda(int code) {
	     to_send += sprintf("\377\376%c", code);	// DON'T xxx
	   },
    "DO":lambda(int code) {
	   to_send += sprintf("\377\374%c", code);	// WON'T xxx
	 },
  ]);

  //. - send_synch
  //.   Sends a TELNET synch command.
  void send_synch()
  {
    // Clear send-queue.
    to_send = "";

    if (fd->write_oob) {
      fd->write_oob("\377");

      fd->write("\362");
    } else {
      // Fallback...
      fd->write("\377\362");
    }
  }

  //. + synch
  //.   Indicates wether we are in synch-mode or not.
  static private int synch = 0;

  //. - got_oob
  //.   Callback called when Out-Of-Band data has been received.
  //. > ignored - The id from the connection.
  //. > s - The Out-Of-Band data received.
  static private void got_oob(mixed ignored, string s)
  {
    DWRITE(sprintf("TELNET: got_oob(\"%s\")\n", s));
    synch = synch || (s == "\377");
    if (cb["URG"]) {
      cb["URG"](id, s);
    }
  }

  //. + rest
  //.   Contains data left over from the line-buffering.
  static private string rest = "";

  //. - got_data
  //.   Callback called when normal data has been received.
  //.   This function also does most of the TELNET protocol parsing.
  //. > ignored - The id from the connection.
  //. > s - The received data.
  static private void got_data(mixed ignored, string s)
  {
    DWRITE(sprintf("TELNET: got_data(\"%s\")\n", s));

    if (sizeof(s) && (s[0] == 242)) {
      DWRITE("TELNET: Data Mark\n");
      // Data Mark handing.
      s = s[1..];
      sync = 0;
    }

    // A single read() can contain multiple or partial commands
    // RFC 1123 4.1.2.10

    array lines = s/"\r\n";

    int lineno;
    for(lineno = 0; lineno < sizeof(lines); lineno++) {
      string line = lines[lineno];
      if (search(line, "\377") != -1) {
	array a = line / "\377";

	string parsed_line = a[0];
	int i;
	for (i=1; i < sizeof(a); i++) {
	  string part = a[i];
	  if (sizeof(part)) {
	    string name = TelnetCodes[part[0]];

	    DWRITE(sprintf("TELNET: Code %s\n", name || "Unknown"));

	    int j;
	    function fun;
	    switch (name) {
	    case 0:
	      // FIXME: Should probably have a warning here.
	      break;
	    default:
	      if (fun = (cb[name] || default_cb[name])) {
		mixed err = catch {
		  fun();
		};
		if (err) {
		  throw(err);
		} else if (!zero_type(err)) {
		  // We were just destructed.
		  return;
		}
	      }
	      a[i] = a[i][1..];
	      break;
	    case "EC":	// Erase Character
	      for (j=i; j--;) {
		if (sizeof(a[j])) {
		  a[j] = a[j][..sizeof(a[j])-2];
		  break;
		}
	      }
	      a[i] = a[i][1..];
	      break;
	    case "EL":	// Erase Line
	      for (j=0; j < i; j++) {
		a[j] = "";
	      }
	      a[i] = a[i][1..];
	      break;
	    case "WILL":
	    case "WON'T":
	    case "DO":
	    case "DON'T":
	      if (fun = (cb[name] || default_cb[name])) {
		fun(a[i][1]);
	      }
	      a[i] = a[i][2..];
	      break;
	    case "DM":	// Data Mark
	      if (sync) {
		for (j=0; j < i; j++) {
		  a[j] = "";
		}
	      }
	      a[i] = a[i][1..];
	      sync = 0;
	      break;
	    }
	  } else {
	    a[i] = "\377";
	    i++;
	  }
	}
	line = a * "";
      }
      if (!lineno) {
	line = rest + line;
      }
      if (lineno < (sizeof(lines)-1)) {
	if ((!sync) && read_cb) {
	  DWRITE(sprintf("TELNET: Calling read_callback(X, \"%s\")\n",
			       line));
	  read_cb(id, line);
	}
      } else {
	DWRITE(sprintf("TELNET: Partial line is \"%s\"\n", line));
	rest = line;
      }
    }
  }

  //. - set_write_callback
  //.   Sets the callback to be called when it is clear to send.
  //. > w_cb - The new read callback.
  void set_write_callback(function(mixed|void:string) w_cb)
  {
    write_cb = w_cb;
    fd->set_nonblocking(got_data, w_cb && send_data, close_cb, got_oob);
  }

  //. - create
  //.   Creates a TELNET protocol handler, and sets its callbacks.
  //. > f - File to use for the connection.
  //. > r_cb - Function to call when data has arrived.
  //. > w_cb - Function to call when data can be sent.
  //. > c_cb - Function to call when the connection is closed.
  //. > callbacks - Mapping with callbacks for the various TELNET commands.
  //. > new_id - Value to send to the various callbacks.
  void create(object f,
	      function(mixed,string:void) r_cb,
	      function(mixed|void:string) w_cb,
	      function(mixed|void:void) c_cb,
	      mapping callbacks, mixed|void new_id)
  {
    fd = f;
    cb = callbacks;

    read_cb = r_cb;
    write_cb = w_cb;
    close_cb = c_cb;
    id = new_id;

    fd->set_nonblocking(got_data, w_cb && send_data, close_cb, got_oob);
  }
}
