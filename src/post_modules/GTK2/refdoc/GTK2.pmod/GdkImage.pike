//! A gdk (low level) image.
//! Mainly used for W(Image) objects.
//!
//! NOIMG
//!
//!

inherit G.Object;

static GDK2.Image create( int|void fast_mode, Image.Image|void image );
//! Create a new GDK2.Image object. The first argument is either 0, which
//! indicates that you want a 'slow' image. If you use '1', you
//! indicate that you want a 'fast' image. Fast images are stored in
//! shared memory, and thus are not sent over any network. But please
//! limit your usage of fast images, they use up a possibly limited
//! system resource set. See the man page for shmget(2) for more
//! information on the limits on shared segments on your system.
//!
//! A 'fast' image will automatically revert back to 'slow' mode if no
//! shared memory is available.
//! 
//! If the second argument is specified, it is the actual image data.
//!
//!

GDK2.Image destroy( );
//! Destructor. Destroys the image. Automatically called by pike when
//! the object is destructed.
//!
//!

int get_pixel( int x, int y );
//! Get the pixel value of a pixel as a X-pixel value.
//! It is usualy not very easy to convert this value to a
//! rgb triple. See get_pnm.
//!
//!

string get_pnm( );
//! Returns the data in the image as a pnm object.
//! Currently, this is always a P6 (true color raw) image.
//! This could change in the future. To get a pike image object
//! do 'Image.PNM.decode( gdkimage->get_pnm() )'
//!
//!

GDK2.Image grab( GTK2.Widget widget, int xoffset, int yoffset, int width, int height );
//! Call this function to grab a portion of a widget (argument 1) to the image.
//! Grabbing non-toplevel widgets may produce unexpected results.
//! To get the size of a widget use -&gt;xsize() and -&gt;ysize().
//! To get the offset of the upper left corner of the widget, relative to it's
//! X-window (this is what you want for the offset arguments), use
//! -&gt;xoffset() and -&gt;yoffset().
//!
//!

GDK2.Image set( Image.Image|int image_or_xsize, int|void ysize );
//! Call this to set this image to either the contents of a pike image
//! or a blank image of a specified size.
//!
//!

GDK2.Image set_pixel( int x, int y, int pixel );
//! Set the pixel value of a pixel. Please note that the pixel argument
//! is a X-pixel value, which is not easily gotten from a RGB color. 
//! See get_pixel and set.
//!
//!
