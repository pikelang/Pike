//! The GdkPixbufAnimation object.  Holds animations, like gifs.
//!
//!

inherit G.Object;

static GDK2.PixbufAnimation create( string filename );
//! Create a new PixbufAnimation.
//!
//!

int get_height( );
//! Returns the height of the bounding box.
//!
//!

GTK2.GdkPixbufAnimationIter get_iter( );
//! Get an iterator for displaying an animation.  The iterator provides the
//! frames that should be displayed at a given time.
//! 
//! Returns the beginning of the animation.  Afterwards you should probably
//! immediately display the pixbuf return by 
//! GDK2.PixbufAnimationIter->get_pixbuf().  Then, you should install a timeout
//! or by some other mechanism ensure that you'll update the image after
//! GDK2.PixbufAnimationIter->get_delay_time() milliseconds.  Each time the
//! image is updated, you should reinstall the timeout with the new, possibly
//! changed delay time.
//! 
//! To update the image, call GDK2.PixbufAnimationIter->advance().
//!
//!

GTK2.GdkPixbuf get_static_image( );
//! If an animation is really just a plain image (has only one frame), this
//! function returns that image.  If the animation is an animation, this
//! function returns reasonable thing to display as a static unanimated image,
//! which might be the first frame, or something more sophisticated.  If an
//! animation hasn't loaded any frames yet, this function will return 0.
//!
//!

int get_width( );
//! Returns the width of the bounding box.
//!
//!

int is_static_image( );
//! If the file turns out to be a plain, unanimated image, this function will
//! return true.  Use get_static_image() to retrieve the image.
//!
//!
