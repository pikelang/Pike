//! An image is a image object stored in client, not X server, memory.
//! A pixmap, on the other hand, is a image object stored in the X-server.
//! See GDK.Image and GDK.Pixmap.
//!
//!@expr{ GTK.Image( GDK.Image(0)->set(Image.image(200,200)->test() ) );@}
//!@xml{<image>../images/gtk_image.png</image>@}
//!
//!
//!
//!

inherit GTK.Misc;

static GTK.Image create( GDK.Image image, GDK.Bitmap mask );
//! You can skip the mask. The mask is used to mask the drawing of the image
//! on it's background. It will not make the container transparent, though.
//! Use shape_combine_mask on the container with the mask for that.
//!
//!

mapping get( );
//! Returns ([ "image":GDK.Image img, "mask":GDK.Bitmap mask ])
//!
//!

GTK.Image set( GDK.Image image, GDK.Bitmap mask );
//! Args: GDK.Image and GDK.Bitmap, respectively.
//! You can skip the mask. The mask is used to mask the drawing of the image
//! on it's background. It will not make the container transparent, though.
//! Use shape_combine_mask on the container with the mask for that.
//! 
//! There is no need to call 'set' with the same GDK.Image again if
//! you have modified the image. You must, however, use -&gt;queue_draw()
//! to redraw the image. Otherwise the old contens will be shown until
//! an refresh of the widget is done for any other reason.
//! 
//!
//!
