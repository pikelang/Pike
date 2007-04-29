//! Properties:
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
//! for the substitute color, all white pixels will become fully
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
//! Creates a transformation of the source image by scaling by scale_x and
//! scale_y, then translating by offset_x and offset_y, then composites
//! the rectangle (dest_x,dest_y,dest_width,dest_height) of the resulting
//! image with a checkboard of the colors color1 and color2 and renders it
//! onto the destinagion image.
//! 
//! See composite_color_simple() for a simpler variant of this function
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

GTK2.GdkPixbuf copy_area( GTK2.GdkPixbuf dest, int src_x, int src_y, int widt, int height, int dest_x, int dest_y );
//! Copies a rectangular area from this pixbuf to dest.  Conversion of
//! pixbuf formats is done automatically.
//!
//!

static GDK2.Pixbuf create( string|mapping options );
//! Create a GDK2.Pixbuf object.  options is either a filename or a mapping
//! of options.  options can be:
//! @xml{<matrix>@}
//! @xml{<r>@}@xml{<c>@}filename@xml{</c>@}@xml{<c>@}name of file to load@xml{</c>@}@xml{<r>@}
//! @xml{<r>@}@xml{<c>@}bits@xml{</c>@}@xml{<c>@}number of bits per sample@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@}width@xml{</c>@}@xml{<c>@}width of image@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@}height@xml{</c>@}@xml{<c>@}height of image@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@}alpha@xml{</c>@}@xml{<c>@}true if alpha channel@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@}scale@xml{</c>@}@xml{<c>@}true if use width and height as scale@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@}preserve@xml{</c>@}@xml{<c>@}true if preserve aspect ratio@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@}xpm@xml{</c>@}@xml{<c>@}if this key exists, then value is xpm data to create from@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@}data@xml{</c>@}@xml{<c>@}if this key exists, then value is pixel data
//! The following is a list of valid keys if key data exists:
//! <table>
//! <tr>@xml{<c>@}colorspace@xml{</c>@}@xml{<c>@}colorspace, default GDK2.COLORSPACE_RGB, currently ignored@xml{</c>@}@xml{</r>@}
//! <tr>@xml{<c>@}rowstride@xml{</c>@}@xml{<c>@}distance in bytes between row starts@xml{</c>@}@xml{</r>@}
//! @xml{</matrix>@}
//! @xml{</c>@}@xml{</r>@}
//!       
//!
//!

GTK2.GdkPixbuf flip( int horizontal );
//! Flips a pixbuf horizontally or vertically and returns the result in
//! a new pixbuf.
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

GTK2.GdkBitmap render_threshold_alpha( int src_x, int src_y, int dest_c, int dest_y, int width, int height, int alpha_threshold );
//! Takes the opacity values in a rectangular portion of a pixbuf and
//! thresholds them to produce a bi-level alpha mask that can be used as a
//! clipping mask for a drawable.
//!
//!

GTK2.GdkPixbuf rotate_simple( int angle );
//! Rotates a pixbuf by a multiple of 90 degrees, and returns the result
//! in a new pixbuf.  angle is either a multiple of 90 degrees (0,90,180,270),
//! or one of @[GDK_PIXBUF_ROTATE_CLOCKWISE], @[GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE], @[GDK_PIXBUF_ROTATE_NONE] and @[GDK_PIXBUF_ROTATE_UPSIDEDOWN], which are merely aliases.
//!
//!

GDK2.Pixbuf saturate_and_pixelate( GTK2.GdkPixbuf dest, float saturation, int pixelate );
//! Modifes saturation and optionally pixelates this pixbuf, placing
//! the result in dest.  The source and dest may be the same pixbuf
//! with no ill effects.  If saturation is 1.0 then saturation is not
//! changed.  If it's less than 1.0, saturation is reduced (the image
//! is darkened); if greater than 1.0, saturation is increased (the image
//! is brightened).  If pixelate is true, then pixels are faded in a
//! checkerboard pattern to create a pixelated image.  This pixbuf and
//! dest must have the same image format, size, and rowstride.
//!
//!

GTK2.GdkPixbuf save( string filename, string type, mapping|void options );
//! Save to a file in format type.  "jpeg", "png", "ico", "bmp",
//! are the only valid writable types at this time.  Quality is only
//! valid for jpeg images.
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

int set_option( string key, string value );
//! Attaches a key/value pair as an option.  If the key already exists
//! in the list of options, the new value is ignored.
//!
//!
