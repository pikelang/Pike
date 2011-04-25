/* Requests.pike
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

class request
{
  constant reqType = 0;
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
    req = ._Xlib.pad(req);
    
    // Big requests extension. Will not work
    // if this extension is not present.
    if((strlen(req)+1) > (65535*4))
      return sprintf("%c%c\0\0%4c%s", reqType, data, 1 + strlen(req) / 4, req);
    return sprintf("%c%c%2c%s", reqType, data, 1 + strlen(req) / 4, req);
  }

  mixed handle_reply(mapping reply)
  {
    error("Xlib.request: unexpected reply!\n");
    return 0;
  }

  mixed handle_error(mapping reply)
  {
    error("Xlib.request: unexpected reply!\n");
    return 0;
  }
}

class FreeRequest
{
  inherit request;
  int id;
  
  string to_string()
  {
    return build_request(sprintf("%4c", id));
  }
}


class CloseFont
{
  inherit FreeRequest;
  constant reqType=46;
}

class FreePixmap
{
  inherit FreeRequest;
  constant reqType=54;
}

class FreeGC
{
  inherit FreeRequest;
  constant reqType=60;
}

class FreeColormap
{
  inherit FreeRequest;
  constant reqType=79;
}


class FreeCursor
{
  inherit FreeRequest;
  constant reqType=95;
}

class FreeColors
{
  inherit request;
  constant reqType=79;
  array colors;
  int colormap;
  int plane_mask;

  string to_string()
  {
    return sprintf("%4c%4c%{%4c%}", colormap,plane_mask,colors);
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
  constant reqType = 1;

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
					   ._Xlib.window_attributes) ),
       depth);
  }
}

class ChangeWindowAttributes
{
  inherit request;
  constant reqType = 2;

  int window;
  mapping attributes = ([ ]);

  string to_string()
  {
    return build_request
      (sprintf("%4c%4c%s", window,
	       @build_value_list(attributes,
				 ._Xlib.window_attributes)));
  }
}

class MapWindow
{
  inherit ResourceReq;
  constant reqType = 8;
}

class UnmapWindow
{
  inherit ResourceReq;
  constant reqType = 10;
}

class GetKeyboardMapping
{
  inherit request;
  constant reqType = 101;
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
  constant reqType = 12;

  int window;
  mapping attributes;

  string to_string()
  {
    return build_request
      (sprintf("%4c%2c\0\0%s", window,
	       @build_value_list(attributes,
				 ._Xlib.window_configure_attributes)));
  }
}

class InternAtom
{
  inherit request;
  constant reqType = 16;

  int onlyIfExists;
  string name;

  string to_string()
  {
    return build_request(sprintf("%2c\0\0%s", strlen(name), name),
			 onlyIfExists);
  }

  mixed handle_reply(mapping reply)
  {
    int id;
    sscanf(reply->rest, "%4c", id);
    return id;
  }

  mixed handle_error(mapping reply)
  {
    switch(reply->errorCode)
      {
      case "Value":
      case "Alloc":
	return reply;
      default:
	error(sprintf("Requests.InternAtom->handle_error: "
		      "Unexpected error '%s'\n", reply->errorCode));
      }
  }
}

class GetAtomName
{
  inherit request;
  constant reqType = 17;

  int atom;

  string to_string()
  {
    return build_request(sprintf("%4c", atom));
  }

  string handle_reply(mapping reply)
  {
    string name;
    int length;
    sscanf(reply->rest, "%2c", length);
    return reply->rest[24..23+length];
  }

  mapping handle_error(mapping reply)
  {
    if (reply->errorCode != "Atom")
      error(sprintf("Requests.GetAtomName->handle_error: "
			"Unexpected error '%s'\n", reply->errorCode));
    return reply;
  }
}

class ChangeProperty
{
  inherit request;
  constant reqType = 18;

  int mode = 0;
  int window;
  int property;
  int type;
  int format;
  string|array(int) data;

  string to_string()
  {
    string p;
    switch(format)
      {
      case 8:
	p = data;
	break;
      case 16:
	p = sprintf("%@2c", data);
	break;
      case 32:
	p = sprintf("%@4c", data);
	break;
      default:
	error(sprintf("Requests.ChangeProperty: Unexpected format %d\n",
			 format));
      }
    return build_request(sprintf("%4c%4c%4c" "%c\0\0\0" "%4c%s",
				 window, property, type,
				 format,
				 sizeof(data), p),
			 mode);
  }
}

class DeleteProperty
{
  inherit request;
  constant reqType = 19;

  int window;
  int property;

  string to_string()
  {
    return build_request(sprintf("%4c%4c", window, property));
  }
}

class GetProperty
{
  inherit request;
  constant reqType = 20;

  int delete;
  int window;
  int property;
  int type;
  int longOffset = 0;
  int longLength = 1024;

  string to_string()
  {
    return build_request(sprintf("%4c%4c%4c" "%4c%4c",
				 window, property, type,
				 longOffset, longLength),
			 delete);
  }

  mapping handle_reply(mapping reply)
  {
    mapping m = ([ "format" : reply->data1 ]);
    int length;
    
    sscanf(reply->rest, "%4c%4c%4c",
	   m->type, m->bytesAfter, length);
    
    /* Match and Property errors (as in rfc-1013) are not used,
     * according to the official specification. For non-existent
     * properties, format == 0 is returned.
     *
     * If types doesn't match, format is non-zero, the actual type is
     * returned, but the data is empty (length = 0). */
     
    if ( (!m->format) || (type && (type != m->type)))
      return 0;
    switch(m->format)
      {
      case 8:
	m->data = reply->rest[24..23+length];
	break;
      case 16:
	{
	  m->data = allocate(length);
	  for (int i = 0; i<length; i++)
	    sscanf(reply->rest[24+2*i..25+2*i], "%2c", m->data[i]);
	  break;
	}
      case 32:
	{
	  m->data = allocate(length);
	  for (int i = 0; i<length; i++)
	    sscanf(reply->rest[24+4*i..27+4*i], "%4c", m->data[i]);
	  break;
	}
      default:
	error(sprintf("Requests.GetProperty->handle_reply: "
		     "Unexpected format %d\n",
		     reply->data1));
      }
    return m;
  }

  mixed handle_error(mapping reply)
  {
    /* If the propert is non-existant, of
     * unexpected type, or too short, return 0 */
    switch (reply->errorCode)
      {
      case "Value":
	return 0;
      case "Atom":
      case "Window":
	return reply;
      default:
	error(sprintf("Requests.GetProperty->handle_error: "
		      "Unexpected error '%s'\n",
		      reply->errorCode));
	
      }
  }
}

class ListProperties
{
  inherit request;
  constant reqType = 21;

  int window;

  string to_string()
  {
    return build_request(sprintf("%4c", window));
  }

  array handle_reply(mapping reply)
  {
    int length;
    sscanf(reply->rest, "%2c", length);

    array a = allocate(length);
    for (int i = 0; i<length; i++)
      sscanf(reply->rest[24+4*i..27+4*i], "%4c", a[i]);

    return a;
  }

  mapping handle_error(mapping reply)
  {
    switch (reply->errorCode)
      {
      case "Window":
	return reply;
      default:
	error(sprintf("Requests.ListProperties->handle_reply: "
		      "Unexpected error '%s'\n",
		      reply->errorCode));
      }
  }
}

#if 0
class GrabPointer
{
  inherit request;
  constant reqType = 26;
}

class UnGrabPointer
{
  inherit request;
  constant reqType = 27;
}
#endif

class GrabButton
{
  inherit request;
  constant reqType = 28;

  int ownerEvents;
  int grabWindow;
  int eventMask;
  int pointerMode;
  int keyboardMode;
  int confineTo;
  int cursor;
  int button;
  int modifiers;

  string to_string()
  {
    return build_request(sprintf("%4c%2c" "%c%c" "%4c%4c%c\0%2c",
				 grabWindow, eventMask,
				 pointerMode, keyboardMode,
				 confineTo, cursor, button, modifiers),
			 ownerEvents);
  }
}

#if 0
class UnGrabButton
{
  inherit request;
  constant reqType = 29;
}
#endif

class OpenFont
{
  inherit request;
  constant reqType = 45;

  int fid;
  string name;

  string to_string()
  {
    return build_request(sprintf("%4c%2c\0\0%s",
				 fid, sizeof(name), name));
  }
}

class QueryTextExtents
{
  inherit request;
  constant reqType = 48;

  int font;
  string str;

  string to_string()
  {
    return build_request(sprintf("%4c%s",
				 font, str), (sizeof(str)/2)&1);
  }

  mapping(string:int) handle_reply(mapping reply)
  {
    mapping(string:int) res = (["DrawDirection":reply->data1]);
    sscanf(reply->rest, "%2c%2c%2c%2c%4c%4c%4c",
	   res->FontAscent, res->FontDescent,
	   res->OverallAscent, res->OverallDescent,
	   res->OverallWidth, res->OverallLeft, res->OverallRight);
    return res;
  }
}

class CreatePixmap
{
  inherit request;
  constant reqType = 53;

  int depth;
  
  int pid;
  int drawable;

  int width, height;

  string to_string()
  {
    return build_request
      (sprintf("%4c%4c" "%2c%2c",
		      pid, drawable,
		      width, height), depth);
  }
}

class CreateGC
{
  inherit request;
  constant reqType = 55;

  int gc;
  int drawable;
  mapping attributes = ([ ]);

  string to_string()
  {
    return build_request(sprintf("%4c%4c%4c%s", gc, drawable,
				 @build_value_list(attributes,
						  ._Xlib.gc_attributes)));
  }
}

class ChangeGC
{
  inherit request;
  constant reqType = 56;

  int gc;
  mapping attributes;

  string to_string()
  {
    return build_request(sprintf("%4c%4c%s", gc, 
				 @build_value_list(attributes,
						  ._Xlib.gc_attributes)));
  }
}

class ClearArea
{
  inherit request;
  constant reqType = 61;

  int exposures;
  int window;
  int x, y;
  int width, height;

  string to_string()
  {
    return build_request(sprintf("%4c%2c%2c%2c%2c",
				 window, x, y, width, height),
			 exposures);
  }
}

class PolyPoint
{
  inherit request;
  constant reqType = 64;

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
  constant reqType = 65;
}

class FillPoly
{
  inherit request;
  constant reqType = 69;

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
  constant reqType = 70;

  int drawable;
  int gc;

  array rectangles;

  string to_string()
  {
    return build_request(sprintf("%4c%4c%s", drawable, gc,
				 rectangles->to_string()*""));
  }
}

class PutImage
{
  inherit request;
  constant reqType = 72;
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
    //    werror(sprintf("PutImage>to_string: %d, %d, %d, %d\n",
    //		   dst_x, dst_y, width, height));
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

class ImageText8
{
  inherit request;
  constant reqType = 76;

  int drawable;
  int gc;
  int x, y;
  string str;

  string to_string()
  {
    return build_request(sprintf("%4c%4c%2c%2c%s",
				 drawable, gc, x, y, str), sizeof(str));
  }
}

class ImageText16
{
  inherit request;
  constant reqType = 77;

  int drawable;
  int gc;
  int x, y;
  string str;

  string to_string()
  {
    return build_request(sprintf("%4c%4c%2c%2c%s",
				 drawable, gc, x, y, str), sizeof(str)/2);
  }
}

class CreateColormap
{
  inherit request;
  constant reqType = 78;

  int cid;
  int alloc;
  
  int window;
  int visual;

  string to_string()
  {
    return build_request(sprintf("%4c%4c%4c", cid, window, visual), alloc);
  }
}

class Bell {
  inherit request;
  constant reqType = 104;
  
  int percent;

  string to_string() { return build_request("", percent); }
}

class CopyArea
{
  inherit request;
  constant reqType = 62;
  object gc;
  object area;
  object src, dst;
  int x,y; 
  
  string to_string()
  {
    return build_request(sprintf("%4c" "%4c" "%4c" "%2c%2c" "%2c%2c" "%2c%2c",
				 src->id, dst->id, gc->id, area->x, area->y,
				 x, y, area->width, area->height));
  }
}

class CopyPlane
{
  inherit CopyArea;
  constant reqType = 63;
  int plane;
  
  string to_string()
  {
    return ::to_string()+sprintf("%4c", plane);
  }
}

class AllocColor
{
  inherit request;
  constant reqType = 84;
  
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

class CreateGlyphCursor
{
  inherit request;
  constant reqType = 94;

  int cid;
  int sourcefont, maskfont;
  int sourcechar, maskchar;
  int forered, foregreen, foreblue;
  int backred, backgreen, backblue;

  string to_string()
  {
    return build_request(sprintf("%4c%4c%4c%2c%2c%2c%2c%2c%2c%2c%2c",
				 cid, sourcefont, maskfont, sourcechar,
				 maskchar, forered, foregreen, foreblue,
				 backred, backgreen, backblue));
  }
}

class QueryExtension
{
  inherit request;
  constant reqType = 98;
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

  void create( int|void m, function|void reply_handler, function|void error_handler )
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
