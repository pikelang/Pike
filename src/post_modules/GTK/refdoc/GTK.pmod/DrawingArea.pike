//! The drawing area is a window you can draw in.
//! Please note that you @b{must@} handle refresh and resize events
//! on your own. Use W(pDrawingArea) for a drawingarea with automatic
//! refresh/resize handling.
//!@expr{ GTK.DrawingArea()->set_usize(100,100)@}
//!@xml{<image>../images/gtk_drawingarea.png</image>@}
//!
//!
//!

inherit GTK.Widget;

GTK.DrawingArea clear( int|void x, int|void y, int|void width, int|void height );
//! Either clears the rectangle defined by the arguments, of if no
//! arguments are specified, the whole drawable.
//!
//!

GTK.DrawingArea copy_area( GDK.GC gc, int xdest, int ydest, GTK.Widget source, int xsource, int ysource, int width, int height );
//! Copies the rectangle defined by xsource,ysource and width,height
//! from the source drawable, and places the results at xdest,ydest in
//! the drawable in which this function is called.
//!
//!

static GTK.DrawingArea create( );
//!

GTK.DrawingArea draw_arc( GDK.GC gc, int filledp, int x1, int y1, int x2, int y2, int angle1, int angle2 );
//! Draws a single circular or elliptical arc.  Each arc is specified
//! by a rectangle and two angles. The center of the circle or ellipse
//! is the center of the rectangle, and the major and minor axes are
//! specified by the width and height.  Positive angles indicate
//! counterclockwise motion, and negative angles indicate clockwise
//! motion. If the magnitude of angle2 is greater than 360 degrees,
//! it is truncated to 360 degrees.
//!
//!

GTK.DrawingArea draw_bitmap( GDK.GC gc, GDK.Bitmap bitmap, int xsrc, int ysrc, int xdest, int ydest, int width, int height );
//! Draw a GDK(Bitmap) in this drawable.
//! @b{NOTE:@} This drawable must be a bitmap as well. This will be
//! fixed in GTK 1.3
//!
//!

GTK.DrawingArea draw_image( GDK.GC gc, GDK.Image image, int xsrc, int ysrc, int xdest, int ydest, int width, int height );
//! Draw the rectangle specified by xsrc,ysrc+width,height from the
//! GDK(Image) at xdest,ydest in the destination drawable
//!
//!

GTK.DrawingArea draw_line( GDK.GC gc, int x1, int y1, int x2, int y2 );
//! img_begin
//! w = GTK.DrawingArea()->set_usize(100,100);
//! delay: g = GDK.GC(w)->set_foreground( GDK.Color(255,0,0) );
//! delay:  for(int x = 0; x<10; x++) w->draw_line(g,x*10,0,100-x*10,99);
//! img_end
//!
//!

GTK.DrawingArea draw_pixmap( GDK.GC gc, GDK.Pixmap pixmap, int xsrc, int ysrc, int xdest, int ydest, int width, int height );
//! Draw the rectangle specified by xsrc,ysrc+width,height from the
//! GDK(Pixmap) at xdest,ydest in the destination drawable
//!
//!

GTK.DrawingArea draw_point( GDK.GC gc, int x, int y );
//! img_begin
//! w = GTK.DrawingArea()->set_usize(10,10);
//! delay: g = GDK.GC(w)->set_foreground( GDK.Color(255,0,0) );
//! delay:  for(int x = 0; x<10; x++) w->draw_point(g, x, x);
//! img_end
//!
//!

GTK.DrawingArea draw_rectangle( GDK.GC gc, int filledp, int x1, int y1, int x2, int y2 );
//! img_begin
//!  w = GTK.DrawingArea()->set_usize(100,100);
//! delay:  g = GDK.GC(w)->set_foreground( GDK.Color(255,0,0) );
//! delay: for(int x = 0; x<10; x++) w->draw_rectangle(g,0,x*10,0,100-x*10,99);
//! img_end
//! img_begin
//! w = GTK.DrawingArea()->set_usize(100,100);
//! delay:   g = GDK.GC(w);
//! delay:  for(int x = 0; x<30; x++) {
//! delay:   g->set_foreground(GDK.Color(random(255),random(255),random(255)) );
//! delay:   w->draw_rectangle(g,1,x*10,0,100-x*10,99);
//! delay: }
//! img_end
//!
//!

GTK.DrawingArea draw_text( GDK.GC gc, GDK.Font font, int x, int y, string text, int forcewide );
//! y is used as the baseline for the text.
//! If forcewide is true, the string will be expanded to a wide string
//! even if it is not already one. This is useful when writing text
//! using either unicode or some other 16 bit font.
//!
//!

GTK.DrawingArea size( int width, int height );
//! This function is OBSOLETE
//!
//!
