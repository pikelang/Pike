/* Shaped windows.
 *
 * $Id: Extensions.pmod,v 1.7 1998/04/19 00:31:00 grubba Exp $

/*
 *    Protocols.X, a Pike interface to the X Window System
 *
 *    Copyright (C) 1998, Niels Möller, Per Hedbor, Marcus Comstedt,
 *    Pontus Hagland, David Hedbor.
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

/* Questions, bug fixes and bug reports can be sent to the pike
 * mailing list, pike@idonex.se, or to the athors (see AUTHORS for
 * email addresses. */

static class extension
{
  object dpy;
  int major, error, event;

  void post_init() {}
  
  int init(object d)
  {
    dpy = d;

    array a =
      d->blocking_request( Requests.QueryExtension( this_object()->name ) );

    if(!a[0]) return 0;

    mapping reply = a[1];
    major = reply->major;
    error = reply->error;
    event = reply->event;
    this_object()->post_init();
    return 1;
  }

  void handle_error(mapping err)
  {

  }

  void handle_event(mapping evnt)
  {

  }
}

class ScreenSaver
{
  inherit extension;
  constant name = "MIT-SCREEN-SAVER";
  

}

class Shape
{
  inherit extension;

  constant name = "SHAPE";
  mapping shape_kind=([ "bounding":0, "clipping":1, "clip":1]);
  mapping shape_op=(["set":0,"union":1,"intersect":2,"subtract":3,"invert":4 ]);
  

  void post_init()
  {
  }

  void ShapeRectangles( object window, int xo, int yo,
			string kind, string operation,
			object(Types.Rectangle)|
			array(object(Types.Rectangle)) rectangles )
  {
    int k = shape_kind[kind];
    int o = shape_op[operation];
    string rects = (objectp(rectangles)?rectangles->to_string():
		    (rectangles->to_string())*"");

    object req = Requests.ExtensionRequest( major, 0, 0 );
    req->code = 1;
    req->data = sprintf("%c%c\0\0%4c%2c%2c%s",o,k,window->id,xo,yo,rects);
    dpy->send_request( req );
  }

  void ShapeMask( object window, int xo, int yo, 
		  string kind, string operation,
		  object|void source )
  {
    int k = shape_kind[kind];
    int o = shape_op[operation];
    
    object req = Requests.ExtensionRequest( major, 0, 0 );
    req->code = 2;
    req->data = 
      sprintf("%c%c\0\0%4c%2c%2c%4c",o,k,window->id,xo,yo,source?source->id:0);
    dpy->send_request( req );
  }

//   void ShapeCombine( object window, string kind, string operation,
// 		     array (object) rectangles )
//   {
//    
//   }

  void ShapeOffset( object window, string kind, int xo, int yo )
  {
    int k = shape_kind[kind];
    
    object req = Requests.ExtensionRequest( major, 0, 0 );
    req->code = 4;
    req->data = sprintf("%c\0\0\0%4c%2c%2c", k, window->id, xo, yo);
    dpy->send_request( req );
  }

//   mapping ShapeQueryExtents( object window )
//   {
    
//   }

//   void ShapeSelectInput( object window, int enable )
//   {

//   }

//   int ShapeInputSelected( object window )
//   {

//   }

//   array (object) ShapeGetRectangles( object window, string kind )
//   {
//
//   }


//   mapping handle_event( mapping evnt )
//   {
//
//   }
}


