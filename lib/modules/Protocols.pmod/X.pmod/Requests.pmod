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
    // Big requests extension. Will not work
    // if this extension is not present.
    if((strlen(req)+1) > (65535*4))
      return sprintf("%c%c\0\0%4c%s", type, data, 1 + strlen(req) / 4, req);
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

class PutImage
{
  inherit request;
  constant type = 72;
  int drawable;
  int gc;

  int depth;
  int width;
  int height;
  int dst_x;
  int dst_y;
  int left_pad = 0;
  int format = 2; // Bitmap XYPixmap ZPixmap
  
  string data;

  string to_string()
  {
  string pad="";
    while(((strlen(data)+strlen(pad))%4)) pad += "\0";
    pad =  build_request(sprintf("%4c" "%4c"
				 "%2c" "%2c"
				 "%2c" "%2c"
				 "%c" "%c" "\0\0"
				 "%s%s",
				 drawable, gc,
				 width, height,
				 dst_x, dst_y, left_pad, depth,
				 data,pad), format);
    data=0;
    return pad;
  }
  
}

class CreateColormap
{
  inherit request;
  constant type = 78;

  int cid;
  int alloc;
  
  int window;
  int visual;

  string to_string()
  {
    return build_request(sprintf("%4c%4c%4c", cid, window, visual), alloc);
  }
}

class AllocColor
{
  inherit request;
  constant type = 84;
  
  int red, green, blue;
  int colormap;
  
  string to_string()
  {
    return build_request(sprintf("%4c%2c%2c%2c\0\0", 
				 colormap, red, green, blue ));
  }

  mixed handle_reply(mapping reply)
  {
    int pixel, r, g, b;
    sscanf(reply->rest, "%2c%2c%2c%*2c%4c", r, g, b, pixel);
    return ([ "pixel":pixel, "red":r, "green":g, "blue":b ]);
  }

  mixed handle_error(string reply)
  {
    return 0;
  }
}

class QueryExtension
{
  inherit request;
  constant type = 98;
  string name;

  void create(string n)
  {
    name = n;
  }
  
  string to_string()
  {
    string pad="";
    while(((strlen(name)+strlen(pad))%4)) pad += "\0";
    return build_request(sprintf("%2c\0\0%s%s", strlen(name),name,pad));
  }

  mapping handle_reply(mapping reply)
  {
    int present, major, event, error_code;
    sscanf(reply->rest, "%c%c%c%c", present, major, event, error_code);
    if(present) return ([ "major":major, "event":event, "error":error_code ]);
  }

  mixed handle_error(string reply)
  {
    return 0;
  }
}




class ExtensionRequest
{
  int type;
  int code;
  string data;
  function handle_reply;
  function handle_error;

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
	  }
	bit <<= 1;
      }
    return ({ mask, sprintf("%@4c", v) });
  }

  string build_request(string req, void|int data)
  {
    return sprintf("%c%c%2c%s", type, data, 1 + strlen(req) / 4, req);
  }


  // End preamble..

  varargs void create( int m, function reply_handler, function error_handler )
  {
    type = m;
    handle_reply = reply_handler;
    handle_error = error_handler;
  }

  string to_string()
  {
    string pad ="";
    if(!data) data = "";
    else while((strlen(data)+strlen(pad))%4) pad += "\0";
    return build_request(data+pad, code);
  }
  
}
