#pike __REAL_VERSION__

//! Display a image on the screen. Requires GTK.

inherit GTK.Window;

typedef Standards.URI|string|Image.Image|Image.Layer|array(Image.Layer) PVImage;
//! The image types accepted. If the image is a string, it is assumed
//! to be a filename of a image that can be loaded with Image.load. 
//! This includes URLs.

enum AlphaMode {
  Squares,
  Solid,
  None,
  AlphaOnly,
};
//! The alpha combination modes.
//!
//! Squares: Checkerboard pattern
//! Solid: Solid color
//! None: Ignore alpha
//! AlphaOnly: Only show the alpha channel, if any.
//!
//! Use @[set_alpha_mode] to change the mode.


#define STEP 30
static {
 GDK.Pixmap pixmap;
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

 void create( PVImage i )
 {
   catch(GTK.setup_gtk());
   ::create( GTK.WindowToplevel );
   set_policy( 0,0,1 );
   show_now();
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
//! for the @[Squares] mode,
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
}

void scale( float factor )
//! Scale the image before display with the specified factor.
{
  scale_factor = factor;
  set_image( old_image );
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
