//! A bitmap is a black and white pixmap. Most commonly used as masks
//! for images, widgets and pixmaps.
//!
//! NOIMG
//!
//!

inherit GDK1.Drawable;

protected GDK1.Bitmap create( int|Image.Image xsize_or_image, int|void ysize, string|void bitmap );
//! Create a new GDK1.Bitmap object.
//! Argument is either an Image.Image object, or {xsisze,ysize,xbitmapdata}.
//!
//!

GDK1.Bitmap destroy( );
//! Destructor. Destroys the bitmap. This will free the bitmap on the X-server.
//!
//!

GDK1.Bitmap ref( );
//! Add a reference
//!
//!

GDK1.Bitmap unref( );
//! Remove a reference
//!
//!
