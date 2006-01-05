//! Properties that can be notified:
//! int bits-per-sample
//! int colorspace (GdkColorspace)
//! int has-alpha
//! int height
//! int n-channels
//! object pixels
//! int rowstride
//! int width
//!
//!

inherit G.Object;

GTK2.GdkPixbuf add_alpha( int substitute_color, int r, int g, int b );
//! Adds an alpha channel to this pixbuf and returns a copy.  If it
//! already has an alpha channel, the channel values are copied from the
//! original; otherwise, the alpha channel is initialized to 255 (full
//! opacity).
//! If substitute_color is true, then the color specified by (r,g,b)
//! will be assigned zero opacity.  That is, if you pass (255,255,255)
//! for the substitute color, all white pixels will become full
//! transparent.
//!
//!

GTK2.GdkPixbuf composite( GTK2.GdkPixbuf dest, int dest_x, int dest_y, int dest_width, int dest_height, float offset_x, float offset_y, float scale_x, float scale_y, int type, int overall_alpha );
//! Creates a transformation of the source image by scalling by scale_x
//! and scale_y, then translating by offset_x and offset_y.  This gives
//! an image in the coordinates of the destination pixbuf.  The rectangle
//! (dest_x,dest_y,dest_width,dest_height) is then composited onto the
//! corresponding rectangle of the original destination image.
//! when the destination rectangle contain parts not in the source image,
//! the data at the edges of the source image is replicated to infinity.
//!
//!

GTK2.GdkPixbuf composite_color( GTK2.GdkPixbuf dest, int dest_x, int dest_y, int dest_width, int dest_height, float offset_x, float offset_y, float scale_x, float scale_y, int type, int overall_alpha, int check_x, int check_y, int check_size, int color1, int color2 );
//! Creates a transformation of the source image by scaling by scale_x
//! and scale_y, then translating by offset_x and offset_y, then
//! composites the rectangle (dest_x,dest_y,dest_width,dest_height) of
//! the resulting image with a checkboard of the colors color1 and color2
//! and renders it onto the destination image.
//! see composite_color_simple() for a simpler variant of this function
//! suitable for many tasks.
//!
//!

GTK2.GdkPixbuf composite_color_simple( int dest_width, int dest_height, int type, int overall_alpha, int check_size, int color1, int color2 );
//! Creates a new W(Pixbuf) by scalling src to dest_width x dest_height
//! and compositing the result with a checkboard of colors color1 and
//! color2.
//!
//!

GTK2.GdkPixbuf copy( );
//! Creates a new GDK2.Pixbuf with a copy of this one.
//!
//!

GTK2.GdkPixbuf copy_area( GTK2.GdkPixbuf dest, int src_x, int src_y, int width, int height, int dest_x, int dest_y );
//! Copies a rectangular area from this pixbuf to dest.  Conversion of
//! pixbuf formats is done automatically.
//!
//!

static GDK2.Pixbuf create( string|int filename_or_alpha, int|mapping bits_or_options, int|void width, int|void height );
//! Create a GDK2.Pixbuf object.  If all parameters are omitted, will create
//! a blank pixbuf with an alpha channel, 8 bits per sample, 320x200.
//!
//!

GDK2.Pixbuf fill( array|mapping|int pixel );
//! Clears a pixbuf to the given RGBA value, converting the RGBA value
//! into the pixbuf's pixel format.  The alpha will be ignored if the
//! pixbuf doesn't have an alpha channel.
//!
//!

int get_bits_per_sample( );
//! Queries the number of bits per color sample.
//!
//!

int get_colorspace( );
//! Queries the color space.
//!
//!

int get_has_alpha( );
//! Queries whether a pixbuf has an alpha channel.
//!
//!

int get_height( );
//! Queries the height.
//!
//!

int get_n_channels( );
//! Queries the number of channels.
//!
//!

string get_option( string key );
//! Looks up key in the list of options that may have been attached
//! to the pixbuf when it was loaded.
//!
//!

string get_pixels( );
//! Returns the pixel data as a string.
//!
//!

int get_rowstride( );
//! Queries the rowstride of a pixbuf, which is the number of bytes
//! between the start of a row and the start of the next row.
//!
//!

int get_width( );
//! Queries the width.
//!
//!

GTK2.GdkPixbuf new_subpixbuf( int src_x, int src_y, int width, int height );
//! Creates a new pixbuf which represents a sub-region of src.  The new
//! pixbuf shares its pixels with the original pixbuf, so writing to one
//! affects both.  The new pixbuf holds a reference to this one, so
//! this object will not be finalized until the new pixbuf is finalized.
//!
//!

int put_pixel( int x, int y, int r, int g, int b );
//! Set pixel to value.
//!
//!

GTK2.GdkBitmap render_threshold_alpha( int src_x, int src_y, int dest_x, int dest_y, int width, int height, int alpha_threshold );
//! Takes the opacity values in a rectangular portion of a pixbuf and
//! thresholds them to produce a bi-level alpha mask that can be used as a
//! clipping mask for a drawable.
//!
//!

GDK2.Pixbuf saturate_and_pixelate( GTK2.GdkPixbuf dest, float saturation, int pixelate );
//! Modifies saturation and optionally pixelates this pixbuf, placing
//! the result in dest.  The source and dest may be the same pixbuf
//! with no ill effects.  If saturation is 1.0 then saturation is not
//! changed.  If it's less than 1.0, saturation is reduced (the image
//! is darkened); if greater than 1.0, saturation is increased (the image
//! is brightened).  If pixelate is true, then pixels are faded in a
//! checkerboard pattern to create a pixelated image.  This pixbuf and
//! dest must have the same image format, size, and rowstride.
//!
//!

int save( string filename, string type, string|void quality );
//! Save to a file in format type.  "jpeg", "png", "ico", "bmp",
//! are the only valid writable types at this time.
//! Quality is only valid for jpeg images.
//!
//!

GTK2.GdkPixbuf scale( GTK2.GdkPixbuf dest, int dest_x, int dest_y, int dest_width, int dest_height, float offset_x, float offset_y, float scale_x, float scale_y, int type );
//! Creates a transformation of the source image by scaling by scale_x
//! and scale_y, then translating by offset_x and offset_y, then renders
//! the rectangle (dest_x,dest_y,dest_width,dest_height) of the
//! resulting image onto the destination image replacing the previous
//! contents.
//! Try to use scale_simple() first, this function is the industrial-
//! strength power tool you can fall back to if scale_simple() isn't
//! powerful enough.
//!
//!

GTK2.GdkPixbuf scale_simple( int dest_width, int dest_height, int|void interp_type );
//! Create a new W(Pixbuf) containing a copy of this W(Pixbuf) scaled to
//! dest_width x dest_height.  Leaves this W(Pixbuf) unaffected.
//! intertype should be GDK2.INTERP_NEAREST if you want maximum speed
//! (but when scaling down GDK2.INTERP_NEAREST is usually unusably ugly).
//! The default interp_type should be GDK2.INTERP_BILINEAR which offers
//! reasonable quality and speed.
//! You can scale a sub-portion by create a sub-pixbuf with new_subpixbuf().
//!
//!

int set_alpha( int x, int y, int setting );
//! Set alpha value.
//!
//!

GDK2.Pixbuf set_from_data( string data, int has_alpha, int width, int height, int rowstride );
//! Creates a new GDK2.Pixbuf out of in-memory image data.
//! Currently only RGB images with 8 bits per sample are supported.
//!
//!

GDK2.Pixbuf set_from_file( string filename );
//! Create a new pixbuf by loading an image from a file.
//!
//!

GDK2.Pixbuf set_from_inline( int data_length, string data );
//! Create a GDK2.Pixbuf from a flat representation that is suitable
//! for storing as inline data in a program.
//! If data_length is -1 it will disable length checks.
//!
//!

GDK2.Pixbuf set_from_xpm_data( array data );
//! Creates a new pixbuf by parsing XPM data in memory.
//!
//!
