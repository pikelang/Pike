/* Shaped windows.
 *
 * $Id$

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

//! an abstract class used to provide features for implimenting
//! X11 extensions. Provides no useful functionality on its own.
static class extension
{
  object dpy;
  int major, error, event;

  void post_init() {}

//! initialize the extension.
//! @param d
//! An object of type Protocols.X.Xlib.Display
  int init(object d)
  {
    dpy = d;

    array a =
      d->blocking_request( .Requests.QueryExtension( this->name ) );

    if(!a[0]) return 0;

    mapping reply = a[1];
    major = reply->major;
    error = reply->error;
    event = reply->event;
    this->post_init();
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
			object(.Types.Rectangle)|
			array(object(.Types.Rectangle)) rectangles )
  {
    int k = shape_kind[kind];
    int o = shape_op[operation];
    string rects = (objectp(rectangles)?rectangles->to_string():
		    (rectangles->to_string())*"");

    object req = .Requests.ExtensionRequest( major, 0, 0 );
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
    
    object req = .Requests.ExtensionRequest( major, 0, 0 );
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
    
    object req = .Requests.ExtensionRequest( major, 0, 0 );
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

//! Provides support for the X11 XTEST extension.
class XTEST
{
  inherit extension;
  constant name="XTEST";

  mapping event_op = (["KeyPress": 2, "KeyRelease": 3, "ButtonPress": 4,
	"ButtonRelease": 5, "MotionNotify": 6]);

  //! Create object.
  void create() {}

  //! Initialize the XTEST extension. Returns 1 if successful.
  //!
  //! @param display
  //!  Protocols.X.Display object
  int init(object display)
  {
    return ::init(display);
  }


  void XTestGetVersion()
  {

  }
  
  void XTestCompareCursor(object window, int cursor)
  {
    
  }

  //! Send a synthetic event to an X server.
  //!
  //! @param event_type
  //!   Type of event to send. Possible values: KeyPress: 2, KeyRelease: 3, 
  //!   ButtonPress: 4, ButtonRelease: 5, MotionNotify: 6
  //!
  //! @param detail
  //!   Button (for Button events) or Keycode (for Key events) to send
  //! @param delay
  //!   Delay before the X server simulates event. 0 indicates zero delay.
  //! @param window
  //!   Window object that a motion event occurrs in. If no window is provided, the root window will be used.
  //! @param xloc
  //!   For motion events, this is the relative X distance or absolute X coordinates.
  //! @param yloc
  //!   For motion events, this is the relative Y distance or absolute Y coordinates.
  void XTestFakeInput(string event_type, int detail, int delay, 
	object|void window, int|void xloc, int|void yloc)
  {

    int e=event_op[event_type];
    int id=0;
    if(window && objectp(window)) id=window->id;

    object req = .Requests.ExtensionRequest( major, 0, 0 );
    req->code=2;

    req->data = sprintf("%c%c\0\0%4c%4c\0\0\0\0\0\0\0\0%2c%2c\0\0\0\0\0\0\0\0",
	e, detail, delay, id, xloc, yloc);
    dpy->send_request( req );

  }

  //! Cause the executing client to become impervious to server grabs.
  //! That is, it can continue to execute requests even if another client 
  //! grabs the server.
  //!
  //! @param impervious 
  //!   A true (non zero) value causes the client to perform as 
  //!   described above. If false (zero), server returns to the normal 
  //!   state of  being susceptible to server grabs.
  void XTestGrabControl(int impervious)
  {

    object req = .Requests.ExtensionRequest( major, 0, 0 );
    req->code=3;

    if(impervious!=0) impervious=1; // if not false, then it's true.

    req->data = sprintf("%c\0\0\0", impervious);
    dpy->send_request( req );
  }
}
