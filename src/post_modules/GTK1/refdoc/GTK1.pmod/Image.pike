//! An image is a image object stored in client, not X server, memory.
//! A pixmap, on the other hand, is a image object stored in the X-server.
//! See GDK1.Image and GDK1.Pixmap.
//!
//!@expr{ GTK1.Image( GDK1.Image(0)->set(Image.Image(200,200)->test() ) );@}
//!@xml{<image>../images/gtk1_image.png</image>@}
//!
//!
//!
//!

inherit GTK1.Misc;

protected GTK1.Image create( GDK1.Image image, GDK1.Bitmap mask );
//! You can skip the mask. The mask is used to mask the drawing of the image
//! on it's background. It will not make the container transparent, though.
//! Use shape_combine_mask on the container with the mask for that.
//!
//!

mapping get( );
//! Returns ([ "image":GDK1.Image img, "mask":GDK1.Bitmap mask ])
//!
//!

GTK1.Image set( GDK1.Image image, GDK1.Bitmap mask );
//! Args: GDK1.Image and GDK1.Bitmap, respectively.
//! You can skip the mask. The mask is used to mask the drawing of the image
//! on it's background. It will not make the container transparent, though.
//! Use shape_combine_mask on the container with the mask for that.
//! 
//! There is no need to call 'set' with the same GDK1.Image again if
//! you have modified the image. You must, however, use -&gt;queue_draw()
//! to redraw the image. Otherwise the old contens will be shown until
//! an refresh of the widget is done for any other reason.
//! 
//!
//!
