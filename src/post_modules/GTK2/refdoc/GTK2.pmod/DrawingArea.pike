//! The drawing area is a window you can draw in.
//! Please note that you @b{must@} handle refresh and resize events
//! on your own. Use W(pDrawingArea) for a drawingarea with automatic
//! refresh/resize handling.
//!@expr{ GTK2.DrawingArea()->set_size_request(100,100)@}
//!@xml{<image>../images/gtk2_drawingarea.png</image>@}
//!
//!
//!

inherit GTK2.Widget;

GTK2.DrawingArea clear( int|void x, int|void y, int|void width, int|void height );
//! Either clears the rectangle defined by the arguments, of if no
//! arguments are specified, the whole drawable.
//!
//!

GTK2.DrawingArea copy_area( GTK2.GdkGC gc, int xdest, int ydest, GTK2.Widget source, int xsource, int ysource, int width, int height );
//! Copies the rectangle defined by xsource,ysource and width,height
//! from the source drawable, and places the results at xdest,ydest in
//! the drawable in which this function is called.
//!
//!

static GTK2.DrawingArea create( mapping|void props );
//! Create a new drawing area.
//!
//!

GTK2.DrawingArea draw_arc( GTK2.GdkGC gc, int filledp, int x1, int y1, int x2, int y2, int angle1, int angle2 );
//! Draws a single circular or elliptical arc.  Each arc is specified
//! by a rectangle and two angles. The center of the circle or ellipse
//! is the center of the rectangle, and the major and minor axes are
//! specified by the width and height.  Positive angles indicate
//! counterclockwise motion, and negative angles indicate clockwise
//! motion. If the magnitude of angle2 is greater than 360 degrees,
//! it is truncated to 360 degrees.
//!
//!

GTK2.DrawingArea draw_bitmap( GTK2.GdkGC gc, GTK2.GdkBitmap bitmap, int xsrc, int ysrc, int xdest, int ydest, int width, int height );
//! Draw a GDK2(Bitmap) in this drawable.
//! @b{NOTE:@} This drawable must be a bitmap as well. This will be
//! fixed in GTK 1.3
//!
//!

GTK2.DrawingArea draw_image( GTK2.GdkGC gc, GTK2.GdkImage image, int xsrc, int ysrc, int xdest, int ydest, int width, int height );
//! Draw the rectangle specified by xsrc,ysrc+width,height from the
//! GDK2(Image) at xdest,ydest in the destination drawable
//!
//!

GTK2.DrawingArea draw_line( GTK2.GdkGC gc, int x1, int y1, int x2, int y2 );
//! img_begin
//! w = GTK2.DrawingArea()->set_size_request(100,100);
//! delay: g = GDK2.GC(w)->set_foreground( GDK2.Color(255,0,0) );
//! delay:  for(int x = 0; x<10; x++) w->draw_line(g,x*10,0,100-x*10,99);
//! img_end
//!
//!

GTK2.DrawingArea draw_pixbuf( GTK2.GdkGC gc, GTK2.GdkPixbuf pixbuf, int xsrc, int ysrc, int xdest, int ydest, int width, int height );
//! Draw a GDK2(Pixbuf) in this drawable.
//!
//!

GTK2.DrawingArea draw_pixmap( GTK2.GdkGC gc, GTK2.GdkPixmap pixmap, int xsrc, int ysrc, int xdest, int ydest, int width, int height );
//! Draw the rectangle specified by xsrc,ysrc+width,height from the
//! GDK2(Pixmap) at xdest,ydest in the destination drawable
//!
//!

GTK2.DrawingArea draw_point( GTK2.GdkGC gc, int x, int y );
//! img_begin
//! w = GTK2.DrawingArea()->set_size_request(10,10);
//! delay: g = GDK2.GC(w)->set_foreground( GDK2.Color(255,0,0) );
//! delay:  for(int x = 0; x<10; x++) w->draw_point(g, x, x);
//! img_end
//!
//!

GTK2.DrawingArea draw_rectangle( GTK2.GdkGC gc, int filledp, int x1, int y1, int x2, int y2 );
//! img_begin
//!  w = GTK2.DrawingArea()->set_size_request(100,100);
//! delay:  g = GDK2.GC(w)->set_foreground( GDK2.Color(255,0,0) );
//! delay: for(int x = 0; x<10; x++) w->draw_rectangle(g,0,x*10,0,100-x*10,99);
//! img_end
//! img_begin
//! w = GTK2.DrawingArea()->set_size_request(100,100);
//! delay:   g = GDK2.GC(w);
//! delay:  for(int x = 0; x<30; x++) {
//! delay:   g->set_foreground(GDK2.Color(random(255),random(255),random(255)) );
//! delay:   w->draw_rectangle(g,1,x*10,0,100-x*10,99);
//! delay: }
//! img_end
//!
//!

GTK2.DrawingArea draw_text( GTK2.GdkGC gc, int x, int y, string text );
//! y is used as the baseline for the text.
//!
//!

GTK2.DrawingArea set_background( GTK2.GdkColor to );
//! Set the background color or image.
//! The argument is either a GDK2.Pixmap or a GDK2.Color object.
//!
//!
