#pike __REAL_VERSION__

//! Display a image on the screen. Requires GTK.

#if constant(GI.repository.Gtk.Application) && constant(Cairo.Context)

import GI.repository;
inherit Gtk.Window;
#define USE_GI

#else

#require constant(GTK2.Window)

#if constant(GTK2.Window)
// Toplevel compat for GTK2
constant GTK = GTK2;
class GDK {
  constant Pixmap = GTK2.GdkPixmap;
  constant Event = GDK2.Event;
}

#define USE_GTK2
#endif

inherit GTK.Window;

#ifdef USE_GTK2
// Compat for old Window functions
void set_background(GTK2.GdkPixmap p) { get_window()->set_background(p); }
GTK2.GdkWindow get_gdkwindow() { return get_window(); }
void set_policy( int a, int b, int c ) { set_resizable(a||b); }
void set_usize(int w, int h) { set_size_request(w, h); }
void set_app_paintable(int flag) { (flag? set_flags:unset_flags)(GTK2.APP_PAINTABLE); }
#endif

#endif

typedef Standards.URI|string|Image.Image|Image.Layer|array(Image.Layer) PVImage;
//! The image types accepted. If the image is a string, it is assumed
//! to be a filename of a image that can be loaded with Image.load.
//! This includes URLs.

//! The alpha combination modes.
//!
//! Use @[set_alpha_mode()] to change the mode.
enum AlphaMode {
  Squares,	//! Checkerboard pattern (default).
  Solid,	//! Solid color.
  None,		//! Ignore alpha.
  AlphaOnly,	//! Only show the alpha channel (if any).
};


#define STEP 30
protected {
#ifdef USE_GI
 Gtk.Widget area;
 Cairo.SurfacePattern image_pattern, squares_pattern;
#else
 GDK.Pixmap pixmap;
#endif
 PVImage old_image;
 AlphaMode alpha_mode;
 Image.Color.Color alpha_color1 = Image.Color.grey,
                         alpha_color2 = Image.Color.darkgrey;

 Image.Image get_alpha_squares()
 {
   Image.Image alpha_sq = Image.Image( STEP*2, STEP*2, alpha_color1 );
   alpha_sq->setcolor( @alpha_color2->rgb() );
   alpha_sq->box( 0, 0, STEP-1, STEP-1 );
   alpha_sq->box( STEP, STEP, STEP+STEP-1, STEP+STEP-1 );
   return alpha_sq;
 }

 Image.Image do_tile_paste( Image.Image bi, Image.Image i, Image.Image a )
 {
   Image.Image q = i->copy();
   for( int y = 0; y<i->ysize(); y+=bi->xsize() )
     for( int x = 0; x<i->xsize(); x+=bi->xsize() )
       q->paste( bi, x, y );
   return q->paste_mask( i, a );
 }

 Image.Image combine_alpha( Image.Image i, Image.Image a )
 {
   if(!a)
     return alpha_mode == AlphaOnly ? i->copy()->clear(255,255,255) : i;

   switch( alpha_mode )
   {
     case Squares:    return do_tile_paste( get_alpha_squares(), i, a );
     case Solid:      return i->clear( alpha_color1)->paste_mask( i, a );
     case None:       return i;
     case AlphaOnly:  return a;
   }
 }

 string _sprintf(int t)
 {
   return t=='O' && sprintf( "%O(%O)", this_program, old_image );
 }

#ifdef USE_GI
 void draw(Gtk.DrawingArea area, Cairo.Context ctx, int w, int h)
 {
   if (!w || !h || !image_pattern) return;
   ctx->set_operator(Cairo.OPERATOR_SOURCE);
   if (squares_pattern) {
     switch( alpha_mode )
     {
       case Squares:    ctx->set_source(squares_pattern); break;
       case Solid:      ctx->set_source_rgb(@alpha_color1->rgbf()); break;
       case AlphaOnly:  ctx->set_source_rgb(0.0, 0.0, 0.0); break;
     }
     ctx->paint();
     ctx->set_operator(Cairo.OPERATOR_OVER);
   } else if (alpha_mode == AlphaOnly) {
     ctx->set_source_rgb(1.0, 1.0, 1.0);
     ctx->paint();
     return;
   }
   Cairo.Matrix m = Cairo.Matrix();
   m->scale(image_pattern->get_surface()->get_width()/(float)w,
            image_pattern->get_surface()->get_height()/(float)h);
   image_pattern->set_matrix(m);
   if (alpha_mode == AlphaOnly) {
     ctx->set_source_rgb(1.0, 1.0, 1.0);
     ctx->mask(image_pattern);
   } else {
     ctx->set_source(image_pattern);
     ctx->paint();
   }
 }
#endif

 void create( PVImage i )
 {
#ifdef USE_GI
   ::create();
   area = Gtk.DrawingArea();
   if (area->set_draw_func)
      area->set_draw_func(draw);
   else
      area->connect("draw", lambda(Gtk.DrawingArea area, Cairo.Context ctx) {
         draw(area, ctx, area->get_allocated_width(), area->get_allocated_height());
      });
   if (::set_child)
      ::set_child(area);
   else {
      area->show();
      ::add(area);
   }
   ::set_title("PV");
#else
   catch(GTK.setup_gtk());
   ::create( GTK.WindowToplevel );
   set_policy( 0,0,1 );
   show_now();

   signal_connect("key_press_event",
                  lambda(GTK.Window win, GDK.Event e) {
                    return key_pressed(e->data);
                  });
#endif
   if( i )
     set_image( i );
 }

 float scale_factor = 1.0;
 int oxs, oys;
}

void set_alpha_mode( AlphaMode m )
//! Set the alpha combining mode. @[m] is one of @[Squares], @[Solid],
//! @[None] and @[AlphaOnly].
{
  alpha_mode = m;
  set_image( old_image );
}

void set_alpha_colors( Image.Color.Color c1, Image.Color.Color|void c2 )
//! Set the colors used for the alpha combination. @[c2] is only used
//! for the @[Squares] alpha mode.
//!
//! @seealso
//!   @[set_alpha_mode()]
{
  if( c1 ) alpha_color1 = c1;
  if( c2 ) alpha_color2 = c2;
  set_image( old_image );
}

Image.Image get_as_image( PVImage i )
//! Return the current image as a Image object, with the alpha
//! combining done.
{
  if( arrayp( i ) )
    i = Image.lay( i );

  if( i->alpha )
    i = combine_alpha( i->image(), i->alpha() );
  return i;
}

void set_image( PVImage i )
//! Change the image.
{
  if( !i ) return;
  int is_uri;
  if( stringp( i ) ||
      (is_uri = (objectp(i) && object_program( i ) >= Standards.URI) ) )
  {
    if( is_uri )
      i = Image.decode_layers( Protocols.HTTP.get_url_data( i ) );
    else
      i = Image.load_layers( i );
  }

  old_image = i;
#ifdef USE_GI
  if( arrayp( i ) )
    i = Image.lay( i );
  int(0..1) has_alpha = 0;
  if (i->alpha && i->alpha()) {
    if (alpha_mode == None || `+(@i->alpha()->min()) == 3*255)
      i = i->image();
    else
      has_alpha = 1;
  }
  image_pattern = Cairo.SurfacePattern(Cairo.ImageSurface(i));
  if (has_alpha) {
    squares_pattern =
      Cairo.SurfacePattern(Cairo.ImageSurface(get_alpha_squares()));
    squares_pattern->set_extend(Cairo.EXTEND_REPEAT);
  } else
    squares_pattern = 0;
  area->set_size_request((int) (i->xsize() * scale_factor),
                         (int) (i->ysize() * scale_factor));
  area->queue_draw();
  ::set_visible(1);
#else
  i = get_as_image( i );
  if( scale_factor != 1.0 )
    i = i->scale( scale_factor );

  int xs = i->xsize(), ys = i->ysize();
  if( (xs != oxs) || (ys != oys) )
  {
    set_app_paintable( 1 );
    set_background( pixmap = GDK.Pixmap( i ) );
    set_usize( xs, ys );
    oxs = xs;
    oys = ys;
  }
  else
    pixmap->set( i );
  GTK.flush();
  while( GTK.main_iteration_do( 0 ) );
  get_gdkwindow()->clear();
  GTK.flush();
  while( GTK.main_iteration_do( 0 ) );
#endif
}

void scale( float factor )
//! Scale the image before display with the specified factor.
{
  scale_factor = factor;
  set_image( old_image );
}

int(1bit) key_pressed(string key)
{
  switch(key)
  {
  case "-": case "<": scale( scale_factor * 0.5 );  break;
  case "+": case ">": scale( scale_factor * 2.0 );  break;
  case ",": scale( scale_factor * 0.9 );  break;
  case ".": scale( scale_factor * 1.1 );  break;
  case "n": scale( 1.0 );  break;
  case "s":
    break;
#if 0
  case "D":
    rm( images[ current_image ] );
    break;
#endif
  case "q":
    _exit(0);
#if 0
  case "c":
    if( copy_to )
      doCopy( images[ current_image ] );
    break;
#endif
#if 0
  case " ":
    current_image+=2;
    // FALLTHRU
  case "\b":
    current_image--;
    current_image = current_image % sizeof( images );
    w->load_image( images[ current_image ] );
    break;
#endif
  }
}

void save( string filename, string|void format )
//! Write the image to a file. If no format is specified, PNG is used.
//! The alpha combination is done on the image before it's saved.
{
  Image.Image i = get_as_image( old_image );
  if( scale_factor != 1.0 )
    i = i->scale( scale_factor );
  Stdio.write_file( filename,
		    Image[upper_case(format)||"PNG"]["encode"]( i ) );
}
