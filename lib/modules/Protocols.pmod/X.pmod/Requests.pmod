/* Requests.pike
 *
 */

#include "error.h"

class request
{
  constant type = 0;
//   constant expect_reply = 0;

  array build_value_list(mapping m, array(string) fields)
  {
    int mask = 0;
    int bit = 1;
    array v = ({ });

    foreach(fields, string f)
      {
	if (!zero_type(m[f]))
	  {
	    v += ({ objectp(m[f]) ? m[f]->id : m[f] });
	    mask |= bit;
// 	    werror(sprintf("Request->build_value_list: field %s, mask = %x\n",
// 			   f, mask));
	  }
	bit <<= 1;
      }
    return ({ mask, sprintf("%@4c", v) });
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

  mixed handle_error(string reply)
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
      (sprintf("%4c%4c" "%2c%2c" "%2c%2c%2c" "%2c%4c" "%4c%s",
		      wid, parent,
		      x, y,
		      width, height, borderWidth,
		      c_class, visual,
		      @build_value_list(attributes,
					_Xlib.window_attributes) ),
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
      (sprintf("%4c%4c%s", window,
	       @build_value_list(attributes,
				 _Xlib.window_attributes)));
  }
}

class MapWindow
{
  inherit ResourceReq;
  constant type = 8;
}

class GetKeyboardMapping
{
  inherit request;
  constant type = 101;
  int first=0;
  int num=0;

  string to_string()
  {
    return build_request( sprintf("%c%c\0\0", first, num) );
  }

  mixed handle_reply(mapping reply)
  {
    int nkc = (reply->data1-4)*4;
    string format;
    if(nkc>0)
      format = "%4c%4c%4c%4c%*"+nkc+"c%s";
    else if(!nkc)
      format = "%4c%4c%4c%4c%s";
    else if(nkc==-4)
      format = "%4c%4c%4c%s";
    else if(nkc==-8)
      format = "%4c%4c%s";
    else if(nkc==-16)
      format = "%4c%s";

    array res = allocate(256);
    reply->rest = reply->rest[24..]; // ?
    for(int i=first; i<num+first; i++)
    {
      array foo = allocate(4);
      sscanf(reply->rest, format, foo[0],foo[1],foo[2],foo[3],reply->rest);
      if(foo[3]==foo[2] && !foo[2])
	foo = foo[..1]+foo[..1];
      res[i]=replace(foo, 0,((foo-({0}))+({0}))[0]);
    }
    return res;
  }

  mixed handle_error(string reply)
  {
    error("Xlib.request: unexpected reply!\n");
    return 0;
  }
}

class ConfigureWindow
{
  inherit request;
  constant type = 12;

  int window;
  mapping attributes;

  string to_string()
  {
    return build_request
      (sprintf("%4c%2c\0\0%s", window,
	       @build_value_list(attributes,
				 _Xlib.window_configure_attributes)));
  }
}

class CreateGC
{
  inherit request;
  constant type = 55;

  int gc;
  int drawable;
  mapping attributes = ([ ]);

  string to_string()
  {
    return build_request(sprintf("%4c%4c%4c%s", gc, drawable,
				 @build_value_list(attributes,
						  _Xlib.gc_attributes)));
  }
}

class ChangeGC
{
  inherit request;
  constant type = 56;

  int gc;
  mapping attributes;

  string to_string()
  {
    return build_request(sprintf("%4c%4c%s", gc, 
				 @build_value_list(attributes,
						  _Xlib.gc_attributes)));
  }
}

class PolyPoint
{
  inherit request;
  constant type = 64;

  int coordMode;
  int drawable;
  int gc;
  array(object) points;
  
  string to_string()
  {
    return build_request(sprintf("%4c%4c%@s", drawable, gc,
				 points->to_string()), coordMode);
  }
}

class PolyLine
{
  inherit PolyPoint;
  constant type = 65;
}

class FillPoly
{
  inherit request;
  constant type = 69;

  int drawable;
  int gc;
  int shape;
  int coordMode;

  array points;

  string to_string()
  {
    return build_request(sprintf("%4c%4c%c%c%2c%@s",
				 drawable, gc, shape, coordMode, 0,
				 points->to_string()));
  }
}

class PolyFillRectangle
{
  inherit request;
  constant type = 70;

  int drawable;
  int gc;

  array rectangles;

  string to_string()
  {
    return build_request(sprintf("%4c%4c%@s", drawable, gc,
				 rectangles->to_string()));
  }
}
