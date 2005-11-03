/* -*- C -*- */
void copy_area(GDK2.GC gc, int xdest, int ydest,
               GTK2.Widget source, int xsource, int ysource,
               int width, int height)
//! Copies the rectangle defined by xsource,ysource and width,height
//! from the source drawable, and places the results at xdest,ydest in
//! the drawable in which this function is called.
{
  struct object *gc, *source;
  GdkWindow *win;
  INT_TYPE xd,yd,xs,ys,w,h;
  get_all_args("copy_area",args,"%o%i%i%o%i%i%i%i",
	       &gc,&xd,&yd,&source,&xs,&ys,&w,&h);

  if (get_gobject(source))
/*    win=get_gdkobject(source,drawable); */
    win=GDK_WINDOW(get_gobject(source));
  else
    win=GTK_WIDGET(get_gobject(source))->window;

  if (xs>=0 && ys>=0 && xd>=0 && yd>=0 && w>0 && h>0)
/*  gdk_window_copy_area( TWIN, GC(gc), xd, yd, win, xs, ys, w, h); */
    gdk_draw_drawable(TWIN,GC(gc),win,xd,yd,xs,ys,w,h);
  RETURN_THIS();
}

void clear(int|void x, int|void y, int|void width, int|void height)
//! Either clears the rectangle defined by the arguments, of if no
//! arguments are specified, the whole drawable.
{
  INT_TYPE x=0,y=0,w=0,h=0;
  if (args==4)
    get_all_args("clear",args,"%i%i%i%i",&x,&y,&w,&h);

  if (x>=0 && y>=0 && w>0 && h>0)
    gdk_window_clear_area(TWIN,x,y,w,h);
  else
    gdk_window_clear(TWIN);

  RETURN_THIS();
}

void draw_point(GDK2.GC gc, int x, int y)
//! img_begin
//! w = GTK2.DrawingArea()->set_size_request(10,10);
//! delay: g = GDK2.GC(w)->set_foreground( GDK2.Color(255,0,0) );
//! delay:  for(int x = 0; x<10; x++) w->draw_point(g, x, x);
//! img_end
{
  struct object *g;
  INT_TYPE x,y;
  get_all_args("draw_point",args,"%o%i%i",&g,&x,&y);

  if (x>=0 && y>=0)
    gdk_draw_point(TWIN,GC(g),x,y);
  RETURN_THIS();
}

void draw_line(GDK2.GC gc, int x1, int y1, int x2, int y2)
//! img_begin
//! w = GTK2.DrawingArea()->set_size_request(100,100);
//! delay: g = GDK2.GC(w)->set_foreground( GDK2.Color(255,0,0) );
//! delay:  for(int x = 0; x<10; x++) w->draw_line(g,x*10,0,100-x*10,99);
//! img_end
{
  struct object *g;
  INT_TYPE x,y,x2,y2;
  get_all_args("draw_line",args,"%o%i%i%i%i",&g,&x,&y,&x2,&y2);

  if (x>=0 && y>=0 && x2>=0 && y2>=0)
    gdk_draw_line(TWIN,GC(g),x,y,x2,y2);
  RETURN_THIS();
}

void draw_rectangle(GDK2.GC gc, int filledp, int x1, int y1, int x2, int y2)
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
{
  struct object *g;
  INT_TYPE f,x,y,x2,y2;
  get_all_args("draw_rectangle",args,"%o%i%i%i%i%i",&g,&f,&x,&y,&x2,&y2);
  if (x>=0 && y>=0 && x2>=0 && y2>=0)
    gdk_draw_rectangle(TWIN,GC(g),f,x,y,x2,y2);
  RETURN_THIS();
}

void draw_arc(GDK2.GC gc, int filledp, int x1, int y1, int x2, int y2,
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
  INT_TYPE f,x,y,x2,y2,a1,a2;
  get_all_args("draw_arc",args,"%o%i%i%i%i%i%i%i",&g,&f,&x,&y,&x2,&y2,
	       &a1,&a2);
  if (x>=0 && y>=0 && x2>=0 && y2>=0)
    gdk_draw_arc(TWIN,GC(g),f,x,y,x2,y2,a1,a2);
  RETURN_THIS();
}

void draw_text(GDK2.GC gc, int x, int y, string text)
//! y is used as the baseline for the text.<br />
{
  struct object *g,*f;
  INT_TYPE x,y;
  struct svalue *sv;
  char *swapped;
  PangoContext *pc;
  PangoLayout *pl;
  char *text;

  get_all_args("draw_text",args,"%o%i%i%s",&g,&x,&y,&text);
  if (x>=0 && y>=0) {
    pc=gtk_widget_get_pango_context(GTK_WIDGET(THIS->obj));
    pl=gtk_widget_create_pango_layout(GTK_WIDGET(THIS->obj),text);
    gdk_draw_layout(TWIN,GC(g),x,y,pl);
    g_object_unref(pc);
    g_object_unref(pl);
  }
  RETURN_THIS();
}

void draw_pixmap(GDK2.GC gc, GDK2.Pixmap pixmap, int xsrc, int ysrc,
                 int xdest, int ydest, int width, int height)
//! Draw the rectangle specified by xsrc,ysrc+width,height from the
//! GDK2(Pixmap) at xdest,ydest in the destination drawable
{
  struct object *g,*p;
  INT_TYPE xs,ys,xd,yd,w,h;
  get_all_args("draw_pixmap",args,"%o%o%i%i%i%i%i%i",
	       &g,&p,&xs,&ys,&xd,&yd,&w,&h);
  if (xs>=0 && ys>=0 && xd>=0 && yd>=0 && w>0 && h>0)
    gdk_draw_pixmap(TWIN,GC(g),GDK_DRAWABLE(p),xs,ys,xd,yd,w,h);
  RETURN_THIS();
}

require gtk22;
void draw_pixbuf(GDK2.GC gc, GDK2.Pixbuf pixbuf, int xsrc, int ysrc,
		 int xdest, int ydest, int width, int height)
//! Draw a GDK2(Pixbuf) in this drawable.<br>
{
  struct object *g,*p;
  INT_TYPE xs,ys,xd,yd,w,h;
  get_all_args("draw_pixbuf",args,"%o%o%i%i%i%i%i%i",
		&g,&p,&xs,&ys,&xd,&yd,&w,&h);
  if (xs>=0 && ys>=0 & xd>=0 && yd>=0 && w>0 && h>0)
    gdk_draw_pixbuf(TWIN,GC(g),GDK_PIXBUF(p),xs,ys,xd,yd,w,h,
		    GDK_RGB_DITHER_NONE,0,0);
  RETURN_THIS();
}
endrequire;

void draw_bitmap(GDK2.GC gc, GDK2.Bitmap bitmap, int xsrc, int ysrc,
                 int xdest, int ydest, int width, int height)
//! Draw a GDK2(Bitmap) in this drawable.<br />
//! <b>NOTE:</b> This drawable must be a bitmap as well. This will be
//! fixed in GTK 1.3
{
  struct object *g, *p;
  INT_TYPE xs, ys, xd, yd, w, h;
  get_all_args("draw_pixmap",args,"%o%o%i%i%i%i%i%i",
	       &g,&p,&xs,&ys,&xd,&yd,&w,&h);

  /* Hm. gdk_draw_bitmap does not exist? Not that it matters. */
/*  gdk_draw_pixmap(TWIN,GC(g),get_gdkobject(p,drawable),xs,ys,xd,yd,w,h); */
  if (xs>=0 && ys>=0 && xd>=0 && yd>=0 && w>0 && h>0)
    gdk_draw_drawable(TWIN,GC(g),GDK_DRAWABLE(p),xs,ys,xd,yd,w,h);
  RETURN_THIS();
}


void draw_image(GDK2.GC gc, GDK2.Image image, int xsrc, int ysrc,
                 int xdest, int ydest, int width, int height)
//! Draw the rectangle specified by xsrc,ysrc+width,height from the
//! GDK2(Image) at xdest,ydest in the destination drawable
{
  struct object *g, *p;
  INT_TYPE xs, ys, xd, yd, w, h;
  get_all_args("draw_image",args,"%o%o%i%i%i%i%i%i",
	       &g,&p,&xs,&ys,&xd,&yd,&w,&h);
  if (xs>=0 && ys>=0 && xd>=0 && yd>=0 && w>0 && h>0)
    gdk_draw_image(TWIN,GC(g),GDK_IMAGE(p),xs,ys,xd,yd,w,h);
  RETURN_THIS();
}

/*
  void draw_polygon( GDK2.GC gc,int filled,
                     array(array(float,size=2)) coordinates)
 gc, filledp, array with xy pairs
*/


void set_background(GDK2.Color|GDK2.Pixmap|GDK2.Bitmap to)
//! Set the background color or image.
//! The argument is either a GDK2.Pixmap or a GDK2.Color object.
{
  pgtk_verify_inited();
  {
    struct object *o1;
    get_all_args("set_background",args,"%o",&o1);

    if (!o1)
      gdk_window_set_back_pixmap(TWIN,NULL,1);
    if (get_pgobject(o1,pgdk_pixmap_program)) {
      gdk_window_set_back_pixmap(TWIN,GDK_PIXMAP(get_gobject(o1)),0);
    } else if (get_pgobject(o1,pgdk_bitmap_program)) {
      gdk_window_set_back_pixmap(TWIN,GDK_DRAWABLE(get_gobject(o1)),0);
    } else if (get_pgobject(o1,pgdk_drawable_program)) {
      gdk_window_set_back_pixmap(TWIN,GDK_DRAWABLE(get_gobject(o1)),0);
    } else if (get_gdkobject(o1,color)) {
      gdk_window_set_background(TWIN,(GdkColor *)get_gdkobject(o1,color));
    } else {
      Pike_error("Set the background to what?\n");
    }
  }
  RETURN_THIS();
}
