//! Pixmaps are data structures that contain pictures. These pictures
//! can be used in various places, but most visibly as icons on the
//! X-Windows desktop, or as cursors. A bitmap is a 2-color pixmap.
//! 
//! To use pixmaps in GTK, you must first build a GDK.Pixmap object
//! using GDK.Pixmap.
//! 
//! The pixels in a GTK.Pixmap cannot be manipulated by the application
//! after creation, since under the X Window system the pixel data is
//! stored on the X server and so is not available to the client
//! application. If you want to create graphical images which can be
//! manipulated by the application, look at W(Image).
//! 
//!@code{ GTK.Pixmap( GDK.Pixmap( Image.image(100,100)->test()) )@}
//!@xml{<image>../images/gtk_pixmap.png</image>@}
//!
//!@code{ GTK.Pixmap( GDK.Pixmap( Image.image(100,100)->test()), GDK.Bitmap(Image.image(100,100,255,255,255)->box(10,10,80,80, 0,0,0) ))@}
//!@xml{<image>../images/gtk_pixmap_2.png</image>@}
//!
//! 
//!
//!

inherit GTK.Misc;

static GTK.Pixmap create( GDK.Pixmap pixmap, GDK.Bitmap mask );
//! Create a new pixmap object, and sets the image and the mask.
//!
//!

mapping get( );
//! Returns ([ 
//!            "pixmap":pixmap, 
//!            "mask":mask,  
//!            "pixmap_insensitive":insensitive version of the pixmap 
//!          ]) 
//!
//!

int get_build_insensitive( );
//!

GTK.Pixmap set( GDK.Pixmap pixmap, GDK.Bitmap mask );
//! Sets the GDK.Pixmap image and the optinal GDK.Bitmap mask
//!
//!

GTK.Pixmap set_build_insensitive( int buildp );
//! Set to TRUE if an extra pixmap should be automatically created to
//! use when the pixmap is insensitive.
//!
//!

GTK.Pixmap set_insensitive_pixmap( GDK.Pixmap insensitive_pixmap );
//! Set the pixmap to use when the pixmap is insensitive.
//!
//!
