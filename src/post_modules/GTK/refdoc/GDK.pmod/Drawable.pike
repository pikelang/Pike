//! The GDK.Bitmap, GDK.Window and GDK.Pixmap classes are all GDK drawables.
//! 
//! This means that you can use the same set of functions to draw in them.
//! 
//! Pixmaps are offscreen drawables. They can be drawn upon with the
//! standard drawing primitives, then copied to another drawable (such
//! as a GDK.Window) with window->draw_pixmap(), set as the background
//! for a window or widget, or otherwise used to show graphics (in a
//! W(Pixmap), as an example). The depth of a pixmap is the number of
//! bits per pixels. Bitmaps are simply pixmaps with a depth of
//! 1. (That is, they are monochrome bitmaps - each pixel can be either
//! on or off).
//! 
//! Bitmaps are mostly used as masks when drawing pixmaps, or as a
//! shape for a GDK.Window or a W(Widget)
//! 
//!
//!

GDK.Drawable clear( int|void x, int|void y, int|void width, int|void height );
//! Either clears the rectangle defined by the arguments, of if no
//! arguments are specified, the whole drawable.
//!
//!

GDK.Drawable copy_area( GDK.GC gc, int xdest, int ydest, GTK.Widget source, int xsource, int ysource, int width, int height );
//! Copies the rectangle defined by xsource,ysource and width,height
//! from the source drawable, and places the results at xdest,ydest in
//! the drawable in which this function is called.
//!
//!

GDK.Drawable draw_arc( GDK.GC gc, int filledp, int x1, int y1, int x2, int y2, int angle1, int angle2 );
//! Draws a single circular or elliptical arc.  Each arc is specified
//! by a rectangle and two angles. The center of the circle or ellipse
//! is the center of the rectangle, and the major and minor axes are
//! specified by the width and height.  Positive angles indicate
//! counterclockwise motion, and negative angles indicate clockwise
//! motion. If the magnitude of angle2 is greater than 360 degrees,
//! it is truncated to 360 degrees.
//!
//!

GDK.Drawable draw_bitmap( GDK.GC gc, GDK.Bitmap bitmap, int xsrc, int ysrc, int xdest, int ydest, int width, int height );
//! Draw a GDK(Bitmap) in this drawable.
//! @b{NOTE:@} This drawable must be a bitmap as well. This will be
//! fixed in GTK 1.3
//!
//!

GDK.Drawable draw_image( GDK.GC gc, GDK.Image image, int xsrc, int ysrc, int xdest, int ydest, int width, int height );
//! Draw the rectangle specified by xsrc,ysrc+width,height from the
//! GDK(Image) at xdest,ydest in the destination drawable
//!
//!

GDK.Drawable draw_line( GDK.GC gc, int x1, int y1, int x2, int y2 );
//! img_begin
//! w = GTK.DrawingArea()->set_usize(100,100);
//! delay: g = GDK.GC(w)->set_foreground( GDK.Color(255,0,0) );
//! delay:  for(int x = 0; x<10; x++) w->draw_line(g,x*10,0,100-x*10,99);
//! img_end
//!
//!

GDK.Drawable draw_pixmap( GDK.GC gc, GDK.Pixmap pixmap, int xsrc, int ysrc, int xdest, int ydest, int width, int height );
//! Draw the rectangle specified by xsrc,ysrc+width,height from the
//! GDK(Pixmap) at xdest,ydest in the destination drawable
//!
//!

GDK.Drawable draw_point( GDK.GC gc, int x, int y );
//! img_begin
//! w = GTK.DrawingArea()->set_usize(10,10);
//! delay: g = GDK.GC(w)->set_foreground( GDK.Color(255,0,0) );
//! delay:  for(int x = 0; x<10; x++) w->draw_point(g, x, x);
//! img_end
//!
//!

GDK.Drawable draw_rectangle( GDK.GC gc, int filledp, int x1, int y1, int x2, int y2 );
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

GDK.Drawable draw_text( GDK.GC gc, GDK.Font font, int x, int y, string text, int forcewide );
//! y is used as the baseline for the text.
//! If forcewide is true, the string will be expanded to a wide string
//! even if it is not already one. This is useful when writing text
//! using either unicode or some other 16 bit font.
//!
//!

mapping get_geometry( );
//! Get width, height position and depth of the drawable as a mapping.
//! 
//! ([ "x":xpos, "y":ypos, "width":xsize, "height":ysize,
//!    "depth":bits_per_pixel ])
//!
//!

int xid( );
//!

int xsize( );
//! Returns the width of the drawable specified in pixels
//!
//!

int ysize( );
//! Returns the height of the drawable specified in pixels
//!
//!
