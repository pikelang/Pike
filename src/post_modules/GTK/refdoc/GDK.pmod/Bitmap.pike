//! A bitmap is a black and white pixmap. Most commonly used as masks
//! for images, widgets and pixmaps.
//!
//! NOIMG
//!
//!

inherit GDK.Drawable;

static GDK.Bitmap create( int|Image.Image xsize_or_image, int|void ysize, string|void bitmap );
//! Create a new GDK.Bitmap object.
//! Argument is either an Image.Image object, or {xsisze,ysize,xbitmapdata}.
//!
//!

GDK.Bitmap destroy( );
//! Destructor. Destroys the bitmap. This will free the bitmap on the X-server.
//!
//!

GDK.Bitmap ref( );
//! Add a reference
//!
//!

GDK.Bitmap unref( );
//! Remove a reference
//!
//!
