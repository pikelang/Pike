//! A bitmap is a black and white pixmap. Most commonly used as masks
//! for images, widgets and pixmaps.
//!
//! NOIMG
//!
//!
inherit GdkDrawable;

static GdkBitmap create( int|object(implements 100) xsize_or_image, int|void ysize, string|void bitmap )
//! Create a new GDK.Bitmap object.
//! Argument is either an Image.image object, or {xsisze,ysize,xbitmapdata}.
//!
//!

GdkBitmap destroy( )
//! Destructor. Destroys the bitmap. This will free the bitmap on the X-server.
//!
//!

GdkBitmap ref( )
//! Add a reference
//!
//!

GdkBitmap unref( )
//! Remove a reference
//!
//!
