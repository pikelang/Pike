#pike __REAL_VERSION__

inherit GTK.DrawingArea;
static object backing_store, bgc;
static int _xsize, _ysize, is_realized;

#define WRAP(X) object X(mixed ... args) \
   {                           \
     if(xsize() != _xsize ||  \
        ysize() != _ysize)   \
       size(xsize(),ysize());                \
     backing_store->X(@args);            \
     refresh();                              \
     return this;                   \
   }

this_program size(int x, int y)
{
  if(!is_realized) return this;
  ::size(x,y);
  if(!bgc) bgc = GDK.GC( background_pix||backing_store||this );
  object nb;
  if((x>_xsize || y>_ysize) && x && y)
    nb = GDK.Pixmap( Image.image(max(x,_xsize),max(y,_ysize)) );
  if(nb && backing_store)
  {
    nb->draw_pixmap( bgc, backing_store, 0,0,0,0, _xsize, _ysize );
    destruct(backing_store);
  }
  if(nb) backing_store = nb;
  if(x>_xsize)
  {
    int ox=_xsize;
    _xsize = x;
    clear(ox, 0, x-ox, max(y,_ysize));
  }
  else if(x<_xsize)
  {
    _xsize = x;
  }

  if(y>_ysize)
  {
    int oy=_ysize;
    _ysize = y;
    clear(0, oy, max(x,_xsize), y-oy);
  }
  else if(y<_ysize)
  {
    _ysize = y;
  }

  refresh();
  return this;
}

//object set_usize(int x, int y)
//{
//  call_out(size,0.01, x,y);
//}

static void rrefresh()
{
  if(!is_realized) return;
  if(xsize() != _xsize || ysize() != _ysize)
    size(xsize(),ysize());
  if(backing_store)
  {
    ::draw_pixmap( bgc, backing_store, 0, 0, 0, 0, _xsize, _ysize);
    GTK.flush();
  }
}

static void refresh()
{
  remove_call_out(rrefresh);
  call_out(rrefresh,0.01);
}

void realized()
{
  call_out(lambda(){is_realized=1;},0);
}

void create()
{
  ::create();
  signal_connect( "expose_event", refresh, 0 );
  signal_connect( "realize", realized, 0 );
//   ::set_usize( 100,100 );
}

GDK.Pixmap background_pix;
GDK.Pixmap background_color;
void set_background( GDK.Pixmap to )
{
  if(to->red)
    background_color = to;
  else
    background_pix = to;
  refresh();
}

this_program clear(int|void x, int|void y, int|void w, int|void h)
{
//   werror("%d,%d->%d,%d <%d,%d>\n", x, y, x+w, y+h, w, h);
  if(xsize() != _xsize ||
     ysize() != _ysize)
    size(xsize(),ysize());

  if(!w && !h)
  {
    w = xsize();
    h = ysize();
  }
  if(background_pix)
  {
    int sx = x+w;
    int stx=x;
    int sy = y+h;
    for(; y<sy; x=stx,y+=background_pix->ysize()-y%background_pix->ysize())
      for(; x<sx; x+=background_pix->xsize()-x%background_pix->xsize())
        backing_store->draw_pixmap(bgc,
                                   background_pix,
                                   (x%background_pix->xsize()),
                                   (y%background_pix->ysize()),
                                   x,y,
                                   min(background_pix->xsize()-
                                       (x%background_pix->xsize()),
                                       sx-x),
                                   min(background_pix->xsize()-
                                       (y%background_pix->xsize()),
                                       sy-y));
  } else if(background_color) {
    bgc->set_foreground( background_color );
    backing_store->draw_rectangle( bgc, 1, x, y, x+w, y+h );
  } else {
//     if(!x && !y && h==_xsize && y == _ysize)
//       backing_store->clear();
//     else
//       backing_store->clear(x,y,h,w);
  }
  refresh();
  return this;
}

WRAP(draw_arc);
WRAP(draw_bitmap);
WRAP(draw_image);
WRAP(draw_line);
WRAP(draw_pixmap);
WRAP(draw_point);
WRAP(draw_rectangle);
WRAP(draw_text);
