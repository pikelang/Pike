/* Xlib.pmod
 *
 */

#define error(x) throw( ({ (x), backtrace() }) )

constant XPORT = 6000;

class rec_buffer
{
  string buffer;
  int expected;
  int pad;

  void create()
  {
    buffer = "";
    expected = pad = 0;
  }

  void expect(int e)
  {
    expected = e;
  }

  int needs()
  {
    return pad + expected - strlen(buffer);
  }
  
  void add_data(string data)
  {
    buffer += data;
  }

  string get_msg()
  {
    if (strlen(buffer) < (expected + pad))
      return 0;
    string res = buffer[pad..pad+expected-1];
    buffer = buffer[pad+expected..];
    pad = (- expected) % 4;
    return res;
  }
}

class Display
{
  import _Xlib;
  
  inherit Stdio.File;
  program Struct = my_struct.struct;
  
  constant STATE_WAIT_CONNECT = 0;
  constant STATE_WAIT_CONNECT_DATA = 1;
  constant STATE_WAIT_CONNECT_FAILURE = 2;
  constant STATE_WAIT_HEADER = 3;
  constant STATE_WAIT_REPLY = 4;
  constant STATE_CLOSED = 4711;

  constant ACTION_CONNECT = 0;
  constant ACTION_CONNECT_FAILED = 1;
  constant ACTION_EVENT = 2;
  constant ACTION_REPLY = 3;
  constant ACTION_ERROR = 4;
  
  int screen_number;

  string buffer;
  object received;
  int state;
  
  function io_error_handler;
  function error_handler;
  function close_handler;
  function connect_handler;
  function event_handler;
  function reply_handler;
  
  /* Information about the X server */
  int majorVersion, minorVersion;  /* Servers protocol version */
  int release;
  int ridBase, ridMask;
  int motionBufferSize;
  string vendor;
  int maxRequestSize;
  array roots;
  array formats;
  int imageByteOrder, bitmapBitOrder;
  int bitmapScanlineUnit, bitmapScanlinePad;
  int minKeyCode, maxKeyCode;
  
  void write_callback()
  {
    int written = write(buffer);
    if (written <= 0)
      {
	if (io_error_handler)
	  io_error_handler(this_object());
	close();
      }
    else
      {
	buffer = buffer[written..];
	if (!strlen(buffer))
	  set_write_callback(0);
      }
  }

  void send(string data)
  {
    buffer += data;
    if (strlen(buffer))
      set_write_callback(write_callback);
  }

  void default_error_handler(object me, mapping e)
  {
    error(sprintf("Xlib: Unhandled error %O\n", e));
  }

  string previous;  /* Partially read reply or event */

  array got_data(string data)
  {
    received->add_data(data);
    string msg;
    while (msg = received->get_msg())
      switch(state)
	{
	case STATE_WAIT_CONNECT: {
	  int success;
	  int len_reason;
	  int length;
	  
	  sscanf(msg, "%c%c%2c%2c%2c",
		 success, len_reason,
		 majorVersion, minorVersion, length);
	  if (success)
	    {
	      received->expect(length*4);
	      state = STATE_WAIT_CONNECT_DATA;
	    } else {
	      received->expect(len_reason);
	      state = STATE_WAIT_CONNECT_FAILURE;
	    }
	  break;
	}
	case STATE_WAIT_CONNECT_FAILURE:
	  return ({ ACTION_CONNECT_FAILED, msg });
	  close();
	  state = STATE_CLOSED;
	  break;
	case STATE_WAIT_CONNECT_DATA: {
	  int nbytesVendor, numRoots, numFormats;
	  object struct  = Struct(msg);

	  release = struct->get_uint(4);
	  ridBase = struct->get_uint(4);
	  ridMask = struct->get_uint(4);
	  motionBufferSize = struct->get_uint(4);
	  nbytesVendor = struct->get_uint(2);
	  maxRequestSize = struct->get_uint(2);
	  numRoots = struct->get_uint(1);
	  numFormats = struct->get_uint(1);
	  imageByteOrder = struct->get_uint(1);
	  bitmapBitOrder = struct->get_uint(1);
	  bitmapScanlineUnit = struct->get_uint(1);
	  bitmapScanlinePad = struct->get_uint(1);
	  minKeyCode = struct->get_uint(1);
	  maxKeyCode = struct->get_uint(1);
	  /* pad2 */ struct->get_fix_string(4);
	  
	  vendor = struct->get_fix_string(nbytesVendor);
	  /* pad */ struct->get_fix_string( (- nbytesVendor) % 4);

	  int i;
	  formats = allocate(numFormats);
	  for(i=0; i<numFormats; i++)
	    {
	      mapping m = ([]);
	      m->depth = struct->get_uint(1);
	      m->bitsPerPixel = struct->get_uint(1);
	      m->scanLinePad = struct->get_uint(1);
	      /* pad */ struct->get_fix_string(5);
	      formats[i] = m;
	    }

	  roots = allocate(numRoots);
	  for(i=0; i<numRoots; i++)
	    {
	      mapping m = ([]);
	      m->windowId = struct->get_uint(4);
	      m->defaultColorMap = struct->get_uint(4);
	      m->whitePixel = struct->get_uint(4);
	      m->blackPixel = struct->get_uint(4);
	      m->currentInputMask = struct->get_uint(4);
	      m->pixWidth = struct->get_uint(2);
	      m->pixHeight = struct->get_uint(2);
	      m->mmWidth = struct->get_uint(2);
	      m->mmHeight = struct->get_uint(2);
	      m->minInstalledMaps = struct->get_uint(2);
	      m->maxInstalledMaps = struct->get_uint(2);
	      m->rootVisualID = struct->get_uint(4);
	      m->backingStore = struct->get_uint(1);
	      m->saveUnders = struct->get_uint(1);
	      m->rootDepth = struct->get_uint(1);
	      int nDepths = struct->get_uint(1);
	      
	      m->depths = allocate(nDepths);
	      for (int j=0; j<nDepths; j++)
		{
		  mapping d = ([]);
		  d->depth = struct->get_uint(1);
		  /* pad */ struct->get_fix_string(1);
		  int nVisuals = struct->get_uint(2);
		  /* pad */ struct->get_fix_string(4);
		  
		  d->visuals = allocate(nVisuals);
		  for(int k=0; k<nVisuals; k++)
		    {
		      mapping v = ([]);
		      v->visualID = struct->get_uint(4);
		      v->c_class = struct->get_uint(1);
		      v->bitsPerRGB = struct->get_uint(1);
		      v->colorMapEntries = struct->get_uint(2);
		      v->redMask = struct->get_uint(4);
		      v->greenMask = struct->get_uint(4);
		      v->blueMask = struct->get_uint(4);
		      /* pad */ struct->get_fix_string(4);
		      d->visuals[k] = v;
		    }
		  m->depths[j] = d;
		}
	      roots[i] = m;
	    }
	  state = STATE_WAIT_HEADER;
	  received->expect(32);
	  return ({ ACTION_CONNECT });
	}
	case STATE_WAIT_HEADER:
	  received->expect(32); /* Correct for most of the cases */
	  switch(msg[0])
	    {
	    case 0: { /* X_Error */
	      mapping m = ([]);
	      sscanf(msg, "%*c%c%2c%4c%2c%c",
		     m->errorCode, m->sequence_number, m->resourceID,
		     m->minorCode, m->majorCode);
	      return ({ ACTION_ERROR, m });
	    }
	    case 1: { /* X_Reply */
	      int length;
	      sscanf(msg[2..3], "%2c", length);
	      if (!length)
		return ({ ACTION_REPLY, msg });

	      state = STATE_WAIT_REPLY;
	      received->expect(length*4);
	      previous = msg;
	      
	      break;
	    }
	    case 11:  /* KeymapNotify */
	      return ({ ACTION_EVENT,
			  ([ "type" : "KeymapNotify",
			   "map" : msg[1..] ]) });
	      
	    default:  /* Any other event */
	      return ({ ACTION_EVENT,
			  ([ "type" : "Unimplemented",
			   "raw" : msg ]) });
	      
	    }
	  break;
	case STATE_WAIT_REPLY:
	  state = STATE_WAIT_HEADER;
	  received->expect(32);
	  return ({ ACTION_REPLY, previous + msg });
	}
    return 0;
  }
  
  void read_callback(mixed id, string data)
  {
    array a = got_data(data);
    if (a)
      switch(a[0])
	{
	case ACTION_CONNECT:
	  connect_handler(this_object(), 1);
	  break;
	case ACTION_CONNECT_FAILED:
	  connect_handler(this_object(), 0, a[1]);
	  break;
	case ACTION_EVENT:
	  event_handler(this_object(), a[1]);
	  break;
	case ACTION_REPLY:
	  reply_handler(this_object(), a[1]);
	  break;
	case ACTION_ERROR:
	  (error_handler || default_error_handler)(this_object(), a[1]);
	  break;
	default:
	  error("Xlib.Display->read_callback: internal error\n");
	  break;
	}
  }

  void close_callback(mixed id)
  {
    if (state == STATE_WAIT_CONNECT)
      connect_handler(this_object(), 0);
    else
      if (close_handler)
	close_handler(this_object());
      else
	io_error_handler(this_object());
    close();
  }
  
  int open(string|void display)
  {
    int async = !!connect_handler;
    
    display = display || getenv("DISPLAY");
    if (!display)
      error("Xlib.pmod: $DISPLAY not set!\n");

    array(string) fields = display_re->split(display);
    if (!fields)
      error("Xlib.pmod: Invalid display name!\n");

    string host = strlen(fields[0]) ? fields[0] : "localhost";

    if (async)
      /* Asynchronous connection */
      set_nonblocking(0, 0, close_callback);
    
    if (!connect(host, XPORT + (int) fields[1]))
      return 0;

    screen_number = (int) fields[2];
    
    buffer = "";
    received = rec_buffer();
    
    /* Always uses network byteorder (big endian) 
     * No authentication */
    string msg = sprintf("B\0%2c%2c%2c%2c\0\0", 11, 0, 0, 0);
    state = STATE_WAIT_CONNECT;
    received->expect(8);
    if (async)
      {
	send(msg);
	set_nonblocking(read_callback, 0, close_callback);
	return 1;
      }
    if (strlen(msg) != write(msg))
      return 0;
    while(1) {
      string data = read(received->needs());
      if (!data)
	return 0;
      array a = got_data(data);
      if (!a)
	continue;
      switch(a[0])
	{
	case ACTION_CONNECT:
	  set_nonblocking(read_callback, 0, close_callback);
	  return 1;
	case ACTION_CONNECT_FAILED:
	  return 0;
	default:
	  error("Xlib.Display->open: Internal error!\n");
	}
    }
  }
}
