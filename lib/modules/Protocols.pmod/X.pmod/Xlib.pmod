/* Xlib.pmod
 *
 */

#include "error.h"

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

class async_request
{
  object req;
  function callback;

  void create(object r, function f)
  {
    req = r;
    callback = f;
  }

  void handle_reply(int success, string reply)
  {
    callback(success, (success ? req->handle_reply : req->handle_error)(reply));
  }
}

class id_manager
{
  int next_id;
  int increment;
  mapping resources;
  
  void create(int base, int mask)
  {
    next_id = base;
    increment = 1;
    while(!(increment & mask))
      increment >>=1;
    resources = ([]);
  }

  int alloc_id()
  {
    int id = next_id;
    next_id += increment;
    return id;
  }

  mixed lookup_id(int i)
  {
    return resources[i];
  }

  void remember_id(int id, mixed resource)
  {
    resources[id] = resource;
  }

  void forget_id(int id)
  {
    m_delete(resources, id);
    /* Could make id available for reallocation */
  }
}

class Display
{
  import _Xlib;
  
  inherit Stdio.File;
  inherit id_manager;
  
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
  int sequence_number;
  
  function io_error_handler;
  function error_handler;
  function close_handler;
  function connect_handler;
  function event_handler;
  function misc_event_handler;
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

  array key_mapping ;

  mapping pending_requests; /* Pending requests */
  object pending_actions;   /* Actions awaiting handling */

  void create() { } /* Delay initialization of id_manager */
  
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
//     werror(sprintf("Xlib.Display->send: '%s'\n", data));
    buffer += data;
    if (strlen(buffer))
      set_write_callback(write_callback);
  }

  int flush()
  { /* FIXME: Not thread-safe */
    set_blocking();
    int result = 0;
    mixed e = catch {
      do {
	int written = write(buffer);
	if (written < 0)
	  break;
	buffer = buffer[written..];
      
	if (strlen(buffer)) 
	  break;
	set_write_callback(0);
	result = 1;
      } while(0);
    };
    set_nonblocking();
    if (e)
      throw(e);
    // werror(sprintf("flush: result = %d\n", result));
    return result;
  }
  
  void default_error_handler(object me, mapping e)
  {
    error(sprintf("Xlib: Unhandled error %O\n", e));
  }

  void default_event_handler(object me, mapping event)
  {
    int wid;
    object w;

    if (event->wid && (w = lookup_id(event->wid))
	&& ((w->event_callbacks["_"+event->type])
	    ||(w->event_callbacks[event->type])))
    {
      if(w->event_callbacks["_"+event->type])
	w->event_callbacks["_"+event->type](event,this_object());
      if(w->event_callbacks[event->type])
	w->event_callbacks[event->type](event,this_object());
    }
    else
      if (misc_event_handler)
	misc_event_handler(event,this_object());
      else
	werror(sprintf("Ignored event %s\n", event->type));
  }
  
  mapping reply;  /* Partially read reply or event */

  void get_keyboard_mapping();

  array process()
  {
    string msg;
    while (msg = received->get_msg())
      switch(state)
	{
	case STATE_WAIT_CONNECT:
	  {
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
	case STATE_WAIT_CONNECT_DATA:
	  {
	    int nbytesVendor, numRoots, numFormats;
	    object struct  = Struct(msg);

	    release = struct->get_uint(4);
	    ridBase = struct->get_uint(4);
	    ridMask = struct->get_uint(4);
	    id_manager::create(ridBase, ridMask);
	    
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
		int wid = struct->get_uint(4);
		object r = Types.RootWindow(this_object(), wid);
		int cm = struct->get_uint(4);
		r->defaultColorMap = Types.Colormap(this_object(), cm);
		r->whitePixel = struct->get_uint(4);
		r->blackPixel = struct->get_uint(4);
		r->currentInputMask = struct->get_uint(4);
		r->pixWidth = struct->get_uint(2);
		r->pixHeight = struct->get_uint(2);
		r->mmWidth = struct->get_uint(2);
		r->mmHeight = struct->get_uint(2);
		r->minInstalledMaps = struct->get_uint(2);
		r->maxInstalledMaps = struct->get_uint(2);
		int rootVisualID = struct->get_uint(4);
		r->backingStore = struct->get_uint(1);
		r->saveUnders = struct->get_uint(1);
		r->rootDepth = struct->get_uint(1);
		int nDepths = struct->get_uint(1);
	      
		r->depths = ([ ]);
		for (int j=0; j<nDepths; j++)
		  {
		    mapping d = ([]);
		    int depth = struct->get_uint(1);
		    /* pad */ struct->get_fix_string(1);
		    int nVisuals = struct->get_uint(2);
		    /* pad */ struct->get_fix_string(4);
		  
		    array visuals = allocate(nVisuals);
		    for(int k=0; k<nVisuals; k++)
		      {
			int visualID = struct->get_uint(4);
			object v = Types.Visual(this_object(), visualID);
			v->c_class = struct->get_uint(1);
			v->bitsPerRGB = struct->get_uint(1);
			v->colorMapEntries = struct->get_uint(2);
			v->redMask = struct->get_uint(4);
			v->greenMask = struct->get_uint(4);
			v->blueMask = struct->get_uint(4);
			/* pad */ struct->get_fix_string(4);
			visuals[k] = v;
		      }
		    r->depths[depth] = visuals;
		  }
		r->visual = lookup_id(rootVisualID);
		roots[i] = r;
	      }
	    if (screen_number >= numRoots)
	      werror("Xlib.Display->process: screen_number too large.\n");
	    state = STATE_WAIT_HEADER;
	    received->expect(32);
	    get_keyboard_mapping();
	    return ({ ACTION_CONNECT });
	  }
	case STATE_WAIT_HEADER:
	  {
	    received->expect(32); /* Correct for most of the cases */
	    string type = _Xlib.event_types[msg[0] & 0x3f];
	    if (type == "Error")
	      {
		mapping m = ([]);
		sscanf(msg, "%*c%c%2c%4c%2c%c",
		       m->errorCode, m->sequenceNumber, m->resourceID,
		       m->minorCode, m->majorCode);
		return ({ ACTION_ERROR, m });
	      }
	    else if (type == "Reply")
	      {
		reply = ([]);
		int length;
	      
		sscanf(msg, "%*c%c%2c%4c%s",
		       reply->data1, reply->sequenceNumber, length,
		       reply->rest);
		if (!length)
		  return ({ ACTION_REPLY, reply });

		state = STATE_WAIT_REPLY;
		received->expect(length*4);
	      }
	    else
	      {
		mapping event = ([ "type" : type ]);
		switch(type)
		  {
		  case "KeyPress":
		  case "KeyRelease":
		  case "ButtonPress":
		  case "ButtonRelease":
		  case "MotionNotify": {
		    int root, child;
		    sscanf(msg, "%*c%c%2c%4c" "%4c%4c%4c"
			        "%2c%2c%2c%2c" "%2c%c",
			   event->detail, event->sequenceNumber, event->time,
			   root, event->wid, child,
			   event->rootX, event->rootY, event->eventx, 
			   event->eventY, event->state, event->sameScreen);
		    event->root = lookup_id(root);
		    event->event = lookup_id(event->wid);
		    event->child = child && lookup_id(child);
		    break;
		  }
#if 0
		  case "EnterNotify":
		  case "LeaveNotify":
		    ...;
		  case "FocusIn":
		  case "FocusOut":
		    ...;
#endif
		  case "KeymapNotify":
		    event->map = msg[1..];
		    break;
		  case "Expose": {
		    sscanf(msg, "%*4c%4c" "%2c%2c%2c%2c" "%2c",
			   event->wid,
			   event->x, event->y, event->width, event->height,
			   event->count);
		    event->window = lookup_id(event->wid);
		    break;
		  }
#if 0		
		  case "GraphicsExpose":
		    ...;
		  case "NoExpose":
		    ...;
		  case "VisibilityNotify":
		    ...;
#endif
		  case "CreateNotify": {
		    int window;
		    sscanf(msg, "%*4c%4c%4c" "%2c%2c" "%2c%2c%2c" "%c",
			   event->wid, window,
			   event->x, event->y,
			   event->width, event->height, event->borderWidth,
			   event->override);
		    event->parent = lookup_id(event->wid);
		    event->window = lookup_id(window) || window;
		    break;
		  }
#if 0
		  case "DestroyNotify":
		    ...;
		  case "UnmapNotify":
		    ...;
#endif
		  case "MapNotify":
		    {
		      int window;
		      sscanf(msg, "%*4c%4c%4c%c",
			     event->wid, window, event->override);
		      event->event = lookup_id(event->wid);
		      event->window = lookup_id(window) || window;
		      break;
		    }
#if 0
		  case "MapRequest":
		    ...;
		  case "ReparentNotify":
		    ...;
#endif
		  case "ConfigureNotify":
		    {
		      int window, aboveSibling;
		      sscanf(msg, "%*4c%4c%4c%4c" "%2c%2c" "%2c%2c%2c" "%c",
			     event->wid, window, aboveSibling,
			     event->x, event->y,
			     event->width, event->height, event->borderWidth,
			     event->override);
		      event->window = lookup_id(window) || window;
		      event->aboveSibling = lookup_id(aboveSibling)
			|| aboveSibling;
		      break;
		    }
#if 0
		  case "ConfigureRequest":
		    ...;
		  case "GravityNotify":
		    ...;
		  case "ResizeRequest":
		    ...;
		  case "CirculateNotify":
		    ...;
		  case "CirculateRequest":
		    ...;
		  case "PropertyNotify":
		    ...;
		  case "SelectionClear":
		    ...;
		  case "SelectionRequest":
		    ...;
		  case "SelectionNotify":
		    ...;
		  case "ColormapNotify":
		    ...;
		  case "ClientMessage":
		    ...;
#endif		
		  case "MappingNotify":
		    get_keyboard_mapping();
		    event = ([ "type" : "MappingNotify",
			       "raw" : msg ]);
		    break;

		  default:  /* Any other event */
		    werror("Unimplemented event: "+
			   _Xlib.event_types[msg[0] & 0x3f]+"\n");
		    event = ([ "type" :type,
			     "raw" : msg ]);
		    break;
		  }
		return ({ ACTION_EVENT, event });
	      }
	    break; 
	  }
	case STATE_WAIT_REPLY:
	  reply->rest += msg;
	  state = STATE_WAIT_HEADER;
	  received->expect(32);
	  return ({ ACTION_REPLY, reply });
	}
    return 0;
  }

  void handle_action(array a)
  {
    // werror(sprintf("Xlib.Display->handle_action: %O\n", a));
    switch(a[0])
      {
      case ACTION_CONNECT:
	connect_handler(this_object(), 1);
	break;
      case ACTION_CONNECT_FAILED:
	connect_handler(this_object(), 0, a[1]);
	break;
      case ACTION_EVENT:
	(event_handler || default_event_handler)(this_object(), a[1]);
	break;
      case ACTION_REPLY:
	{
	  mixed handler = pending_requests[a[1]->sequenceNumber];
	  if (handler)
	    {
	      m_delete(pending_requests, a[1]->sequenceNumber);
	      handler->handle_reply(1, a[1]);
	    }
	  else if(reply_handler)
	    reply_handler(this_object(), a[1]);
	  else
	    werror("Unhandled reply: "+a[1]->sequenceNumber+"\n");
	  break;
	}
      case ACTION_ERROR:
	{
	  mixed handler = pending_requests[a[1]->sequenceNumber];
	  if (handler)
	    {
	      m_delete(pending_requests, a[1]->sequenceNumber);
	      handler->handle_reply(0, a[1]);
	    }
	  else
	    (error_handler || default_error_handler)(this_object(), a[1]);
	  break;
	}
      default:
	error("Xlib.Display->handle_action: internal error\n");
	break;
      }
  }
  
  void read_callback(mixed id, string data)
  {
    received->add_data(data);
    array a;
    while(a = process())
      handle_action(a);
  }

  void process_pending_actions()
  {
    array a;
    while(a = pending_actions->get())
      handle_action(a);
    set_read_callback(read_callback);
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

    if (!connect(host, XPORT + (int)fields[1]))
      return 0;

    /* Asynchronous connection */
    if (async)
      set_nonblocking(0, 0, close_callback);

    screen_number = (int) fields[2];
    
    buffer = "";
    received = rec_buffer();
    pending_requests = ([]);
    pending_actions = ADT.queue();
    sequence_number = 1;
    
    /* Always uses network byteorder (big endian) 
     * No authentication */
    string msg = sprintf("B\0%2c%2c%2c%2c\0\0", 11, 0, 0, 0);
    state = STATE_WAIT_CONNECT;
    received->expect(8);
    send(msg);
    if (async)
      {
	set_read_callback(read_callback);
	return 1;
      }
    if (!flush())
      return 0;
    while(1) {
      string data = read(received->needs());
      if (!data)
	return 0;
      received->add_data(data);
      array a = process();
      if (a)
	switch(a[0])
	  {
	  case ACTION_CONNECT:
	    get_keyboard_mapping();
	    set_nonblocking(read_callback, 0, close_callback);
	    return 1;
	  case ACTION_CONNECT_FAILED:
	    werror("Connection failed: "+a[1]+"\n");
	    return 0;
	  default:
	    error("Xlib.Display->open: Internal error!\n");
	  }
    }
  }

  int send_request(object req, mixed|void handler)
  {
    send(req->to_string());
    return sequence_number++;
  }

  mixed blocking_request(object req)
  {
    mixed result = 0;
    int done = 0;

    int n = send_request(req);
    flush();
    set_blocking();

    mixed e = catch {
      while(!done)
	{
	  string data = read(0x7fffffff, 1);
	  if (!data)
	    break;
	  received->add_data(data);
	  array a;
	  while (!done && (a = process()))
	    {
	      if ((a[0] == ACTION_REPLY)
		  && (a[1]->sequenceNumber == n))
		{
		  result = req->handle_reply(1,a[1]);
		  done = 1;
		  break;
		}
	      else if ((a[0] == ACTION_ERROR)
		       && (a[1]->sequenceNumber == n))
		{
		  result = req->handle_error(1,a[1]);
		  done = 1;
		  break;
		}
	      /* Enqueue all other actions */
	      pending_actions->put(a);
	    }
	}
    };
    set_nonblocking();
    if (e)
      throw(e);
    if (pending_actions->size())
      {
	set_read_callback(0);
	call_out(process_pending_actions, 0);
      }
    return result;
  }
  
  void send_async_request(object req, function callback)
  {
    int n = send_request(req);
    pending_requests[n] = async_request(req, callback);
  }

  void got_mapping(int ok, array foo)
  {
    key_mapping = foo;
  }

  void get_keyboard_mapping()
  {
    object r = Requests.GetKeyboardMapping();
    r->first = minKeyCode;
    r->num = maxKeyCode - minKeyCode + 1;
    send_async_request(r, got_mapping);
  }

  object DefaultRootWindow()
  {
    return roots[screen_number];
  }
}
