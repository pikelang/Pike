/* Requests.pike
 *
 */

#define error(x) throw( ({ (x), backtrace() }) )

class request
{
  constant type = 0;
  constant expect_reply = 0;

  string build_value_list(mapping m, array(string) fields)
  {
    int mask = 0;
    int bit = 1;
    array v = ({ });

    foreach(fields, string f)
      {
	if (!zero_type(m[f]))
	  {
	    v += ({ m[f] });
	    mask |= bit;
	    werror(sprintf("Request->build_value_list: field %s, mask = %x\n",
			   f, mask));
	  }
	bit <<= 1;
      }
    return sprintf("%4c%@4c", mask, v);
  }

  string build_request(string req, void|int data)
  {
    if (strlen(req) % 4)
      error("Xlib.request: internal error!\n");
    return sprintf("%c%c%2c%s", type, data, 1 + strlen(req) / 4, req);
  }

  mixed handle_reply(string reply)
  {
    error("Xlib.request: unexpected reply!\n");
    return 0;
  }
}

class ResourceReq
{
  inherit request;
  int id;

  string to_string()
  {
    return build_request(sprintf("%4c", id));
  }
}

class CreateWindow
{
  inherit request;
  constant type = 1;

  int depth;
  
  int wid;
  int parent;

  int x, y;
  int width, height, borderWidth;

  int c_class;
  int visual;

  mapping attributes = ([ ]);

  string to_string()
  {
    return build_request
      (sprintf("%4c%4c" "%2c%2c" "%2c%2c%2c" "%2c%4c" "%s",
		      wid, parent,
		      x, y,
		      width, height, borderWidth,
		      c_class, visual,
		      build_value_list(attributes, _Xlib.window_attributes) ),
       depth);
  }
}

class ChangeWindowAttributes
{
  inherit request;
  constant type = 2;

  int window;
  mapping attributes = ([ ]);

  string to_string()
  {
    return build_request
      (sprintf("%4c%s", window,
	       build_value_list(attributes, _Xlib.window_attributes)));
  }
}

class MapWindow
{
  inherit ResourceReq;
  constant type = 8;
}
