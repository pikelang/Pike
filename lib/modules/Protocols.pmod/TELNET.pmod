//
// $Id: TELNET.pmod,v 1.3 1998/04/23 21:24:50 grubba Exp $
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

//. Implements TELNET as described by RFC 764 and RFC 854
//.
//. Also implements the Q method of TELNET option negotiation
//. as specified by RFC 1143.



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

  //. + option_states
  //.   Negotiation state of all WILL/WON'T options.
  static private array(int) option_states = allocate(256);

  // See RFC 1143 for the use and meaning of these.
  constant option_us_yes	= 0x0001;
  constant option_us_want	= 0x0002;
  constant option_us_opposite	= 0x0010;

  constant option_him_yes	= 0x0100;
  constant option_him_want	= 0x0200;
  constant option_him_opposite	= 0x1000;

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

  //. - enable_option
  //.   Starts negotiation to enable a TELNET option.
  //. > option - The option to enable.
  void enable_option(int option)
  {
    if ((option < 0) || (option > 255)) {
      throw(({ sprintf("Bad TELNET option #%d\n", option), backtrace() }));
    }
    switch(option_states[option] & 0xff00) {
    case 0x0000: /* NO */
    case 0x1000: /* NO OPPOSITE */
      option_states[option] &= ~option_him_opposite;
      option_states[option] |= option_him_want | option_him_yes;
      to_send += sprintf("\377\375%c", option); // DO option
      break;
    case 0x0100: /* YES */
    case 0x1100: /* YES OPPOSITE */
      /* Error: Already enabled */
      break;
    case 0x0200: /* WANTNO EMPTY */
      /* Error: Cannot initiate new request in the middle of negotiation. */
      /* FIXME: **********************/
      break;
    case 0x1200: /* WANTNO OPPOSITE */
      /* Error: Already queued an enable request */
      break;
    case 0x1300: /* WANTYES OPPOSITE */
      /* Error: Already negotiating for enable */
      break;
    case 0x0300: /* WANTYES EMPTY */
      option_states[option] &= ~option_him_opposite;
      break;
    }
  }

  //. - disable_option
  //.   Starts negotiation to disable a TELNET option.
  //. > option - The option to disable.
  void disable_option(int option)
  {
    if ((option < 0) || (option > 255)) {
      throw(({ sprintf("Bad TELNET option #%d\n", option), backtrace() }));
    }
    switch(option_states[option] & 0xff00) {
    case 0x0000: /* NO */
    case 0x1000: /* NO OPPOSITE */
      /* Error: Already disabled */
      break;
    case 0x0100: /* YES */
    case 0x1100: /* YES OPPOSITE */
      option_states[option] &= ~(option_him_opposite | option_him_yes);
      option_states[option] |= option_him_want;
      to_send += sprintf("\377\376%c", option); // DON'T option
      break;
    case 0x0200: /* WANTNO EMPTY */
      /* Error: Already negotiating for disable */
      break;
    case 0x1200: /* WANTNO OPPOSITE */
      option_states[option] &= ~option_him_opposite;
      break;
    case 0x1300: /* WANTYES OPPOSITE */
      /* Error: Cannot initiate new request in the middle of negotiation. */
      /* FIXME: **********************/
      break;
    case 0x0300: /* WANTYES EMPTY */
      /* Error: Already queued an disable request */
      break;
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
      synch = 0;
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
	      int option = a[i][1];
	      int state = option_states[option];
	      a[i] = a[i][2..];

	      DWRITE(sprintf("WILL %d, state 0x%04x\n", option, state));

	      switch(state & 0xff00) {
	      case 0x0000: /* NO */
	      case 0x1000: /* NO OPPOSITE */
		if ((fun = (cb["WILL"] || default_cb["WILL"])) &&
		    fun(option)) {
		  /* Agree about enabling */
		  option_states[option] |= option_him_yes;
		  to_send += sprintf("\377\375%c", option); // DO option
		} else {
		  to_send += sprintf("\377\376%c", option); // DON'T option
		}
		break;
	      case 0x0100: /* YES */
	      case 0x1100: /* YES OPPOSITE */
		/* Ignore */
		break;
	      case 0x0200: /* WANTNO EMPTY */
		state &= ~option_him_want;
		if ((fun = (cb["Enable_Option"] ||
			    default_cb["Enable_Option"]))) {
		  fun(option);
		}
		break;
	      case 0x1200: /* WANTNO OPPOSITE */
		state &= ~(option_him_yes|option_him_opposite);
		to_send += sprintf("\377\376%c", option); // DON'T option
		break;
	      case 0x0300: /* WANTYES EMPTY */
	      case 0x1300: /* WANTYES OPPOSITE */
		  // Error: DON'T answered by WILL
		  option_states[option] &= ~0x1200;
		  option_states[option] |= option_him_yes;
		  if ((fun = (cb["Enable_Option"] ||
			      default_cb["Enable_Option"]))) {
		    fun(option);
		  }
		break;
	      }

	      state = option_states[option];
	      DWRITE(sprintf("=> WILL %d, state 0x%04x\n", option, state));

	      break;
	    case "WON'T":
	      int option = a[i][1];
	      int state = option_states[option];
	      a[i] = a[i][2..];

	      DWRITE(sprintf("WON'T %d, state 0x%04x\n", option, state));

	      switch(state & 0xff00) {
	      case 0x0000: /* NO */
	      case 0x1000: /* NO OPPOSITE */
		/* Ignore */
		break;
	      case 0x0100: /* YES */
	      case 0x1100: /* YES OPPOSITE */
		option_states[option] &= ~0xff00;
		to_send += sprintf("\377\376%c", option); // DON'T option
		if ((fun = (cb["Disable_Option"] ||
			    default_cb["Disable_Option"]))) {
		  fun(option);
		}
		break;
	      case 0x0200: /* WANTNO EMPTY */
	      case 0x0300: /* WANTYES EMPTY */
	      case 0x1300: /* WANTYES OPPOSITE */
		option_states[option] &= ~0xff00;
		break;
	      case 0x1200: /* WANTNO OPPOSITE */
		option_states[option] &= ~option_him_opposite;
		option_states[option] |= option_him_yes;
		to_send += sprintf("\377\375%c", option); // DO option
		break;
	      }

	      state = option_states[option];
	      DWRITE(sprintf("=> WON'T %d, state 0x%04x\n", option, state));

	      break;
	    case "DO":
	      int option = a[i][1];
	      int state = option_states[option];
	      a[i] = a[i][2..];

	      DWRITE(sprintf("DO %d, state 0x%04x\n", option, state));

	      switch(state & 0xff) {
	      case 0x00: /* NO */
	      case 0x10: /* NO OPPOSITE */
		if ((fun = (cb["DO"] || default_cb["DO"])) &&
		    fun(option)) {
		  /* Agree about enabling */
		  DWRITE("AGREE\n");
		  option_states[option] |= option_us_yes;
		  to_send += sprintf("\377\373%c", option); // WILL option
		} else {
		  DWRITE("DISAGREE\n");
		  to_send += sprintf("\377\374%c", option); // WON'T option
		}
		break;
	      case 0x01: /* YES */
	      case 0x11: /* YES OPPOSITE */
		/* Ignore */
		break;
	      case 0x02: /* WANTNO EMPTY */
		state &= ~option_us_want;
		if ((fun = (cb["Enable_Option"] ||
			    default_cb["Enable_Option"]))) {
		  fun(option);
		}
		break;
	      case 0x12: /* WANTNO OPPOSITE */
		state &= ~(option_us_yes|option_us_opposite);
		to_send += sprintf("\377\374%c", option); // WON'T option
		break;
	      case 0x03: /* WANTYES EMPTY */
	      case 0x13: /* WANTYES OPPOSITE */
		  option_states[option] &= ~0x12;
		  option_states[option] |= option_us_yes;
		  if ((fun = (cb["Enable_Option"] ||
			      default_cb["Enable_Option"]))) {
		    fun(option);
		  }
		break;
	      }

	      state = option_states[option];
	      DWRITE(sprintf("=> DO %d, state 0x%04x\n", option, state));

	      break;
	    case "DON'T":
	      int option = a[i][1];
	      int state = option_states[option];
	      a[i] = a[i][2..];

	      DWRITE(sprintf("DON'T %d, state 0x%04x\n", option, state));

	      switch(state & 0xff) {
	      case 0x00: /* NO */
	      case 0x10: /* NO OPPOSITE */
		/* Ignore */
		break;
	      case 0x01: /* YES */
	      case 0x11: /* YES OPPOSITE */
		option_states[option] &= ~0xff;
		to_send += sprintf("\377\374%c", option); // WON'T option
		if ((fun = (cb["Disable_Option"] ||
			    default_cb["Disable_Option"]))) {
		  fun(option);
		}
		break;
	      case 0x02: /* WANTNO EMPTY */
	      case 0x03: /* WANTYES EMPTY */
	      case 0x13: /* WANTYES OPPOSITE */
		option_states[option] &= ~0xff;
		break;
	      case 0x12: /* WANTNO OPPOSITE */
		option_states[option] &= ~option_us_opposite;
		option_states[option] |= option_us_yes;
		to_send += sprintf("\377\373%c", option); // WILL option
		break;
	      }

	      state = option_states[option];
	      DWRITE(sprintf("=> DON'T %d, state 0x%04x\n", option, state));

	      break;
	    case "DM":	// Data Mark
	      if (synch) {
		for (j=0; j < i; j++) {
		  a[j] = "";
		}
	      }
	      a[i] = a[i][1..];
	      synch = 0;
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
	if ((!synch) && read_cb) {
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
