/* Xlib.pmod
 *
 * $Id$
 */

/*
 *    Protocols.X, a Pike interface to the X Window System
 *
 *    See COPYRIGHT for copyright information.
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA 
 */

#pike __REAL_VERSION__

// #define DEBUG

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
    return pad + expected - sizeof(buffer);
  }
  
  void add_data(string data)
  {
    buffer += data;
  }

  string get_msg()
  {
    if (sizeof(buffer) < (expected + pad))
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

  void handle_reply(mapping reply)
  {
    callback(1, req->handle_reply(reply));
  }

  void handle_error(mapping reply)
  {
    callback(0, req->handle_error(reply));
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
  import ._Xlib;
  
  inherit Stdio.File;
  inherit id_manager;
  inherit .Atom.atom_manager;
  

  
  void close_callback(mixed id);
  void read_callback(mixed id, string data);

  // FIXME! Should use some sort of (global) db.
  mapping compose_patterns;
  
  program Struct = ADT.struct;
  
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

  mapping extensions = ([]); // all available extensions.

  mapping pending_requests; /* Pending requests */
  object pending_actions;   /* Actions awaiting handling */

#ifdef DEBUG
  mapping debug_requests = ([ ]);
# define DEBUGREQ(X) ((X)&0xfff)
#endif
  
  void create()
  { /* Delay initialization of id_manager */
    compose_patterns = ([]);
    catch {
      compose_patterns = decode_value(Stdio.read_bytes("db/compose.db"));
    };
    atom_manager::create();
  }
  
  void write_callback()
  {
    if (sizeof(buffer))
      {
	int written = write(buffer);
	if (written < 0)
	  {
	    if (io_error_handler)
	      io_error_handler(this);
	    close();
	  }
	else
	  {
	    // werror(sprintf("Xlib: wrote '%s'\n", buffer[..written-1]));
	    buffer = buffer[written..];
	    // if (!sizeof(buffer))
	    //   set_write_callback(0);
	  }
      }
  }

  void send(string data)
  {
    int ob = sizeof(buffer);
    buffer += data;
    if (!ob && sizeof(buffer))
      {
#if 0
	set_write_callback(write_callback);
	/* Socket is most likely non-blocking,
	 * and write_callback() expects to be be able to write
	 * at least one character. So don't call it yet. */
#endif
	write_callback( ); 
      }
  }

  /* This function leaves the socket in blocking mode */
  int flush()
  { /* FIXME: Not thread-safe */
    set_blocking();
    int written = write(buffer);
    if (written < sizeof(buffer))
      return 0;
    buffer = "";

    // werror(sprintf("flush: result = %d\n", result));
    //     trace(0);
    return 1;
  }
  
  void default_error_handler(object me, mapping e)
  {
    if(e->failed_request)
      e->failed_request = e->failed_request[0..32];
    error(sprintf("Xlib: Unhandled error %O\n", e));
  }

  void default_event_handler(object me, mapping event)
  {
    int wid;
    object w;
    array a;
    
//     werror(sprintf("Event: %s\n", event->type));
    if (event->wid && (w = lookup_id(event->wid))
	&& (a = (w->event_callbacks[event->type])))
      {
	foreach(a, array pair)
	  {
	    if (!(event = pair[1](event, this)))
	      return;
	  }
	// FIXME: Should event be forwarded to parent or not?
	// No, let the widget's signal() function handle that.
      }
    else
      if (misc_event_handler)
	misc_event_handler(event,this);
//       else
// 	werror(sprintf("Ignored event %s\n", event->type));
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
		object r = .Types.RootWindow(this, wid);
		int cm = struct->get_uint(4);
		r->colormap = r->defaultColorMap = .Types.Colormap(this, cm, 0);
		r->colormap->autofree=0;
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
			object v = .Types.Visual(this, visualID);

			v->depth = depth;
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

	    foreach(values(.Extensions), program p)
	      {
		object e = p();
		if(e->init( this ))
		  extensions[e->name] = e;
		else
		  destruct( e ) ;
	      }
	    return ({ ACTION_CONNECT });
	  }
	case STATE_WAIT_HEADER:
	  {
	    received->expect(32); /* Correct for most of the cases */
	    string type = ._Xlib.event_types[msg[0] & 0x3f];
	    if (type == "Error")
	      {
		mapping m = ([]);
		int errorCode;
		sscanf(msg, "%*c%c%2c%4c%2c%c",
		       errorCode, m->sequenceNumber, m->resourceID,
		       m->minorCode, m->majorCode);
		m->errorCode = ._Xlib.error_codes[errorCode];
#ifdef DEBUG
		m->failed_request = debug_requests[DEBUGREQ(m->sequenceNumber)];
#endif
#if 0
		if (m->errorCode == "Success")
		  {
		    werror("Xlib.Display->process: Error 'Success' ignored.\n");
		    break;
		  }
#endif
// 		werror(sprintf("Xlib: Received error %O\n", m));
		return ({ ACTION_ERROR, m });
	      }
	    else if (type == "Reply")
	      {
		// werror(sprintf("Xlib.Display->process: Reply '%s'\n",
		// 	       msg));
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
		  case "MotionNotify":
		    {
		      int root, child;
		      sscanf(msg, "%*c%c%2c%4c" "%4c%4c%4c"
			     "%2c%2c%2c%2c" "%2c%c",
			     event->detail, event->sequenceNumber, event->time,
			     root, event->wid, child,
			     event->rootX, event->rootY, event->eventX, 
			     event->eventY, event->state, event->sameScreen);
		      event->root = lookup_id(root);
		      event->event = lookup_id(event->wid);
		      event->child = child && lookup_id(child);
		      break;
		    }
		  case "EnterNotify":
		  case "LeaveNotify":
		    {
		      int root, child, flags;
		      sscanf(msg, "%*4c%4c%4c%4c%4c" "%2c%2c%2c%2c" "%2c%c%c",
			     event->time, root, event->wid, child,
			     event->rootX, event->rootY, event->eventX, event->eventY,
			     event->state, event->mode, flags);
		      event->root = lookup_id(root);
		      event->event = lookup_id(event->wid);
		      event->child = child && lookup_id(child);
		      event->flags = (< >);
		      if (flags & 1)
			event->flags->Focus = 1;
		      if (flags & 2)
			event->flags->SameScreen = 1;
		      break;
		    }
#if 0
		  case "FocusIn":
		  case "FocusOut":
		    ...;
#endif
		  case "KeymapNotify":
		    event->map = msg[1..];
		    break;
		  case "Expose":
		    {
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
		  case "CreateNotify":
		    {
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
		  case "DestroyNotify":
		    {
		      int window;
		      sscanf(msg, "%*4c%4c%4c",
			     event->wid, window);
		      event->window = lookup_id(window) || window;
		      break;
		    }
#if 0
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
// 		    werror("Unimplemented event: "+
// 			   ._Xlib.event_types[msg[0] & 0x3f]+"\n");
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
	connect_handler(this, 1);
	break;
      case ACTION_CONNECT_FAILED:
	connect_handler(this, 0, a[1]);
	break;
      case ACTION_EVENT:
	(event_handler || default_event_handler)(this, a[1]);
	break;
      case ACTION_REPLY:
	{
	  mixed handler = pending_requests[a[1]->sequenceNumber];
	  if (handler)
	    {
	      m_delete(pending_requests, a[1]->sequenceNumber);
	      handler->handle_reply(a[1]);
	    }
	  else if(reply_handler)
	    reply_handler(this, a[1]);
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
	      handler->handle_error(a[1]);
	    }
	  else
	    (error_handler || default_error_handler)(this, a[1]);
	  break;
	}
      default:
	error("Xlib.Display->handle_action: internal error\n");
	break;
      }
  }
  void read_callback(mixed id, string data)
  {
    // werror(sprintf("Xlib: received '%s'\n", data));
    received->add_data(data);
    array a;
    while(a = process())
      handle_action(a);
  }

  void close_callback(mixed id)
  {
    if (state == STATE_WAIT_CONNECT)
      connect_handler(this, 0);
    else
      if (close_handler)
	close_handler(this);
      else
	exit(0);
    close();
  }
  
  void process_pending_actions()
  {
    array a;
    while(a = pending_actions->get())
      handle_action(a);
    set_nonblocking(read_callback, write_callback, close_callback);
  }
	  
  int open(string|void display)
  {
    int async = !!connect_handler;
    int is_local;
    int display_number;
    
    display = display || getenv("DISPLAY");
    if (!display)
      error("Xlib.pmod: $DISPLAY not set!\n");

    array(string) fields = display_re->split(display);
    if (!fields)
      error("Xlib.pmod: Invalid display name!\n");

    if (1 != sscanf(fields[1], "%d", display_number))
      error(sprintf("Xlib.Display->open: Invalid display number '%s'.\n",
		    fields[1]));

    string host;
    if (sizeof(fields[0]))
      host = fields[0];
    else
      {
	if(File::open("/tmp/.X11-pipe/X"+((int)display_number), "rw"))
	  {
// 	    werror("Using local transport\n");
	    is_local = 1;
	  } else {
// 	    werror("Failed to use local transport.\n");
	    host = "127.0.0.1";
	  }
      }
    /* Authentication */

    mapping auth_data;
    object auth_file = .Auth.read_auth_data();

    if (auth_file)
      auth_data = host ? auth_file->lookup_ip(gethostbyname(host)[1][0],
					      display_number)
	: auth_file->lookup_local(gethostname(), display_number);
    else
      werror("Xlib: Could not read auth-file\n");

    if (!auth_data)
      auth_data = ([ "name" : "", "data" : ""]);
    
    /* Asynchronous connection */
    if (async)
    {
       if (!is_local)
	  open_socket();
       set_nonblocking(0, 0, close_callback);
    }
    if(!is_local)
    {
       int port =  XPORT + (int)fields[1];
//  	werror(sprintf("Xlib: Connecting to %s:%d\n", host, port));
       if (!connect(host, port))
       {
//    	  werror(sprintf("Xlib: Connecting to %s:%d failed\n", host, port));
	  return 0;
       }
//  	werror(sprintf("Xlib: Connected to %s:%d\n", host, port));
    }

    set_buffer( 65536 );

    if(sizeof(fields[2]))
      screen_number = (int) fields[2][1..];
    else
      screen_number = 0;

    buffer = "";
    received = rec_buffer();
    pending_requests = ([]);
    pending_actions = ADT.Queue();
    sequence_number = 1;
    
    /* Always uses network byteorder (big endian) */
    string msg = sprintf("B\0%2c%2c%2c%2c\0\0%s%s",
			 11, 0,
			 sizeof(auth_data->name), sizeof(auth_data->data),
			 ._Xlib.pad(auth_data->name), ._Xlib.pad(auth_data->data));

    state = STATE_WAIT_CONNECT;
    received->expect(8);
    send(msg);
    if (async)
      {
	set_nonblocking(read_callback, write_callback, close_callback);
	return 1;
      }
    if (!flush())
      return 0;
    
    int result = 0;
    int done = 0;

    while(1)
      {
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
	      set_nonblocking(read_callback, write_callback, close_callback);
	      return 1;
	    case ACTION_CONNECT_FAILED:
 	      werror("Connection failed: "+a[1]+"\n");
	      return 0;
	      break;
	    default:
	      error("Xlib.Display->open: Internal error!\n");
	    }
      }
  }

  int send_request(object req, mixed|void handler)
  {
    string data = req->to_string();
    send(data);
#ifdef DEBUG
    debug_requests[DEBUGREQ(sequence_number)] = data;
#endif
    return (sequence_number++)&0xffff; // sequence number is just 2 bytes
  }

  array blocking_request(object req)
  {
    int success;
    mixed result = 0;
    int done = 0;

    int n = send_request(req);
    flush();

    while(!done)
    {
       string data = read(0x7fffffff, 1);
       if (!data)
       {
	  call_out(close_callback, 0);
	  return ({ 0, 0 });
       }
       received->add_data(data);
       array a;
       while (a = process())
       {
	  if ((a[0] == ACTION_REPLY)
	      && (a[1]->sequenceNumber == n))
	  {
	     result = req->handle_reply(a[1]);
	     done = 1;
	     success = 1;
	     break;
	  }
	  else if ((a[0] == ACTION_ERROR)
		   && (a[1]->sequenceNumber == n))
	  {
	     result = req->handle_error(a[1]);
	     done = 1;
	     break;
	  }
	  /* Enqueue all other actions */
	  pending_actions->put(a);
       }
    }

    if (!pending_actions->is_empty())
      call_out(process_pending_actions, 0);
    else
      set_nonblocking(read_callback,write_callback,close_callback);

    return ({ success, result });
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
    object r = .Requests.GetKeyboardMapping();
    r->first = minKeyCode;
    r->num = maxKeyCode - minKeyCode + 1;
    send_async_request(r, got_mapping);
  }

  object DefaultRootWindow()
  {
    return roots[screen_number];
  }

  object OpenFont_req(string name)
  {
    object req = .Requests.OpenFont();
    req->fid = alloc_id();
    req->name = name;
    return req;
  }

  mapping (string:object) fonts = ([]);

  object OpenFont(string name)
  {
    if(fonts[name]) return fonts[name];
    object req = OpenFont_req(name);
    send_request(req);
    fonts[name] = .Types.Font(this, req->fid);
    return fonts[name];
  }

  object CreateGlyphCursor_req(object sourcefont, object maskfont,
			       int sourcechar, int maskchar,
			       array(int) foreground, array(int) background)
  {
    object req = .Requests.CreateGlyphCursor();
    req->cid = alloc_id();
    req->sourcefont = sourcefont->id;
    req->maskfont = maskfont->id;
    req->sourcechar = sourcechar;
    req->maskchar = maskchar;
    req->forered = foreground[0];
    req->foregreen = foreground[1];
    req->foreblue = foreground[2];
    req->backred = background[0];
    req->backgreen = background[1];
    req->backblue = background[2];
    return req;
  }

  object CreateGlyphCursor(object sourcefont, int sourcechar,
			   object|void maskfont, int|void maskchar,
			   array(int)|void foreground,
			   array(int)|void background)
  {
    if(!maskfont) {
      maskfont = sourcefont;
      if(!maskchar)
	maskchar = sourcechar + 1;
    }
    object req = CreateGlyphCursor_req(sourcefont, maskfont,
				       sourcechar, maskchar,
				       foreground||({0,0,0}),
				       background||({0xffff,0xffff,0xffff}));
    send_request(req);
    return .Types.Cursor(this, req->cid);
  }
  
  object Bell_req(int volume)
  {
    object req=.Requests.Bell();
    req->percent=volume;
    return req;
  }
  
  void Bell(int volume)
  {
    send_request(Bell_req(volume));
  }
}
