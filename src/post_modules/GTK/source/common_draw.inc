/* -*- C -*- */
void copy_area( GDK.GC gc, int xdest, int ydest,
                GTK.Widget source, int xsource, int ysource,
                int width, int height )
//! Copies the rectangle defined by xsource,ysource and width,height
//! from the source drawable, and places the results at xdest,ydest in
//! the drawable in which this function is called.
{
  struct object *gc, *source;
  GdkWindow *win;
  INT_TYPE xd, yd, xs, ys, w, h;
  get_all_args("copy_area",args, "%o%i%i%o%i%i%i%i",
	       &gc, &xd, &yd, &source, &xs, &ys, &w, &h);

  if(get_gdkobject( source, drawable ))
    win = get_gdkobject( source, drawable );
  else
    win = GTK_WIDGET( get_gtkobject( source ) )->window;

  gdk_window_copy_area( TWIN, GC(gc), xd, yd, win, xs, ys, w, h);
  RETURN_THIS();
}

void clear( int|void x, int|void y, int|void width, int|void height )
//! Either clears the rectangle defined by the arguments, of if no
//! arguments are specified, the whole drawable.
{
  INT_TYPE x=0, y=0, w=0, h=0;
  if(args==4)
    get_all_args("clear", args, "%i%i%i%i", &x, &y, &w, &h);

  if(h)
    gdk_window_clear_area(TWIN, x,y,w,h);
  else
    gdk_window_clear(TWIN);

  RETURN_THIS();
}

void draw_point(GDK.GC gc, int x, int y)
//! img_begin
//! w = GTK.DrawingArea()->set_usize(10,10);
//! delay: g = GDK.GC(w)->set_foreground( GDK.Color(255,0,0) );
//! delay:  for(int x = 0; x<10; x++) w->draw_point(g, x, x);
//! img_end
{
  struct object *g;
  INT_TYPE x, y;
  get_all_args("draw_point", args, "%o%i%i", &g, &x, &y);

  gdk_draw_point( TWIN, GC(g), x, y );
  RETURN_THIS();
}

void draw_line(GDK.GC gc, int x1, int y1, int x2, int y2)
//! img_begin
//! w = GTK.DrawingArea()->set_usize(100,100);
//! delay: g = GDK.GC(w)->set_foreground( GDK.Color(255,0,0) );
//! delay:  for(int x = 0; x<10; x++) w->draw_line(g,x*10,0,100-x*10,99);
//! img_end
{
  struct object *g;
  INT_TYPE x, y, x2, y2;
  get_all_args("draw_line", args, "%o%i%i%i%i", &g, &x, &y, &x2, &y2);

  gdk_draw_line( TWIN, GC(g), x, y, x2, y2 );
  RETURN_THIS();
}

void draw_rectangle(GDK.GC gc, int filledp, int x1, int y1, int x2, int y2)
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
{
  struct object *g;
  INT_TYPE f, x, y, x2, y2;
  get_all_args("draw_rectangle",args, "%o%i%i%i%i%i",&g, &f, &x, &y, &x2, &y2);
  gdk_draw_rectangle( TWIN, GC(g), f, x, y, x2, y2 );
  RETURN_THIS();
}

void draw_arc(GDK.GC gc, int filledp, int x1, int y1, int x2, int y2,
              int angle1, int angle2)
//! Draws a single circular or elliptical arc.  Each arc is specified
//! by a rectangle and two angles. The center of the circle or ellipse
//! is the center of the rectangle, and the major and minor axes are
//! specified by the width and height.  Positive angles indicate
//! counterclockwise motion, and negative angles indicate clockwise
//! motion. If the magnitude of angle2 is greater than 360 degrees,
//! it is truncated to 360 degrees.
{
  struct object *g;
  INT_TYPE f, x, y, x2, y2, a1, a2;
  get_all_args("draw_arc",args, "%o%i%i%i%i%i%i%i",&g, &f, &x, &y, &x2, &y2,
	       &a1, &a2);
  gdk_draw_arc( TWIN, GC(g), f, x, y, x2, y2, a1, a2 );
  RETURN_THIS();
}

void draw_text(GDK.GC gc, GDK.Font font, int x, int y,
               string text, int forcewide )
//! y is used as the baseline for the text.<br />
//! If forcewide is true, the string will be expanded to a wide string
//! even if it is not already one. This is useful when writing text
//! using either unicode or some other 16 bit font.
{
  struct object *g,*f;
  INT_TYPE x, y, force_wide=0;
  struct pike_string *s;
  char *swapped;
  get_all_args("draw_text",args, "%o%o%i%i%W",&g,&f,  &x, &y, &s);
  gdk_draw_text( TWIN, get_gdkobject(f,font), get_gdkobject(g,gc),
		   x, y, s->str, s->len<<s->size_shift );
  RETURN_THIS();
}

void draw_pixmap(GDK.GC gc, GDK.Pixmap pixmap, int xsrc, int ysrc,
                 int xdest, int ydest, int width, int height)
//! Draw the rectangle specified by xsrc,ysrc+width,height from the
//! GDK(Pixmap) at xdest,ydest in the destination drawable
{
  struct object *g, *p;
  INT_TYPE xs, ys, xd, yd, w, h;
  get_all_args("draw_pixmap",args,"%o%o%i%i%i%i%i%i",
	       &g,&p,&xs,&ys,&xd,&yd,&w,&h);
  gdk_draw_pixmap(TWIN,GC(g),get_gdkobject(p,drawable),xs,ys,xd,yd,w,h);
  RETURN_THIS();
}

void draw_bitmap(GDK.GC gc, GDK.Bitmap bitmap, int xsrc, int ysrc,
                 int xdest, int ydest, int width, int height)
//! Draw a GDK(Bitmap) in this drawable.<br />
//! <b>NOTE:</b> This drawable must be a bitmap as well. This will be
//! fixed in GTK 1.3
{
  struct object *g, *p;
  INT_TYPE xs, ys, xd, yd, w, h;
  get_all_args("draw_pixmap",args,"%o%o%i%i%i%i%i%i",
	       &g,&p,&xs,&ys,&xd,&yd,&w,&h);
  /* Hm. gdk_draw_bitmap does not exist? Not that it matters. */
  gdk_draw_pixmap(TWIN,GC(g),get_gdkobject(p,drawable),xs,ys,xd,yd,w,h);
  RETURN_THIS();
}

void draw_image(GDK.GC gc, GDK.Image image, int xsrc, int ysrc,
                 int xdest, int ydest, int width, int height)
//! Draw the rectangle specified by xsrc,ysrc+width,height from the
//! GDK(Image) at xdest,ydest in the destination drawable
{
  struct object *g, *p;
  INT_TYPE xs, ys, xd, yd, w, h;
  get_all_args("draw_image",args,"%o%o%i%i%i%i%i%i",
	       &g,&p,&xs,&ys,&xd,&yd,&w,&h);
  gdk_draw_image(TWIN,GC(g),get_gdkobject(p,image),xs,ys,xd,yd,w,h);
  RETURN_THIS();
}

/*
  void draw_polygon( GDK.GC gc,int filled,
                     array(array(float,size=2)) coordinates)
 gc, filledp, array with xy pairs
*/


