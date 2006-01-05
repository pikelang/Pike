//! A bitmap is a black and white pixmap. Most commonly used as masks
//! for images, widgets and pixmaps.
//!
//! NOIMG
//!
//!

inherit GDK2.Drawable;

static GDK2.Bitmap create( int|Image.Image xsize_or_image, int|void ysize, string|void bitmap );
//! Create a new GDK2.Bitmap object.
//! Argument is either an Image.image object, or {xsisze,ysize,xbitmapdata}.
//!
//!

GDK2.Bitmap destroy( );
//! Destructor. Destroys the bitmap. This will free the bitmap on the X-server.
//!
//!

GDK2.Bitmap ref( );
//! Add a reference
//!
//!

GDK2.Bitmap unref( );
//! Remove a reference
//!
//!
