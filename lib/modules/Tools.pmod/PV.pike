//! Display a image on the screen. Requires GTK.

inherit GTK.Window;

typedef string|object(Image.Image)|object(Image.Layer)|array(Image.Layer) PVImage;
//! The image types accepted. If the image is a string, it is assumed
//! to be a filename of a image that can be loaded with Image.load. 
//! This includes URLs.

enum AlphaMode {
  Squares,
  Solid,
  None,
  AlphaOnly,
};

AlphaMode alpha_mode;

//! Use set_alpha_mode to change the alpha combining mode.

#define STEP 30
static GDK.Pixmap pixmap;
static PVImage old_image;

void set_alpha_mode( AlphaMode m )
{
  alpha_mode = m;
  set_image( old_image );
}

void set_alpha_colors( object c1, object|void c2 )
{
  if( c1 ) alpha_color1 = c1;
  if( c2 ) alpha_color2 = c2;
  set_image( old_image );
}

object alpha_color1 = Image.Color.grey,
            alpha_color2 = Image.Color.darkgrey;

static Image.Image get_alpha_squares() 
{
  Image.Image alpha_sq = Image.Image( STEP*2, STEP*2, alpha_color1 );
  alpha_sq->setcolor( @alpha_color2->rgb() );
  alpha_sq->box( 0, 0, STEP-1, STEP-1 );
  alpha_sq->box( STEP, STEP, STEP+STEP-1, STEP+STEP-1 );
  return alpha_sq;
}

static Image.Image do_tile_paste( Image.Image bi, Image.Image i, Image.Image a )
{
  Image.Image q = i->copy();
  for( int y = 0; y<i->ysize(); y+=bi->xsize() )
    for( int x = 0; x<i->xsize(); x+=bi->xsize() )
      q->paste( bi, x, y );
  return q->paste_mask( i, a );
}

static Image.Image combine_alpha( Image.Image i, Image.Image a )
{
  if(!a) return i;

  switch( alpha_mode )
  {
   case Squares:    return do_tile_paste( get_alpha_squares(), i, a );
   case Solid:      return i->clear( alpha_color1)->paste_mask( i, a );
   case None:       return i;
   case AlphaOnly:  return a;
  }
}

void set_image( PVImage i )
//! Update the image.
{
  if( !i ) return;
  old_image = i;
  if( stringp( i ) )
    i = Image.load_layers( i );

  if( arrayp( i ) )
    i = Image.lay( i );

  if( i->alpha )
    i = combine_alpha( i->image(), i->alpha() );

  set_usize( i->xsize(), i->ysize() );
  set_background( pixmap = GDK.Pixmap( i ) );
  get_gdkwindow()->clear();
  GTK.flush();
  while( GTK.main_iteration_do( 0 ) );
}

static string _sprintf( )
{
  return sprintf( "Tools.PV(%O)", old_image );
}

static void create(  PVImage i )
{
  catch(GTK.setup_gtk());
  ::create( GTK.WindowToplevel );
  set_policy( 0,0,1 );
  show_now();
  if( i )
    set_image( i );
}
