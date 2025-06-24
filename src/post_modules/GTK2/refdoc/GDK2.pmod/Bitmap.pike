// Automatically generated from "gdkbitmap.pre".
// Do NOT edit.

//! A bitmap is a black and white pixmap. Most commonly used as masks
//! for images, widgets and pixmaps.
//!
//! NOIMG
//!
//!

inherit GDK2.Drawable;
//!

protected void create( void xsize_or_image, void ysize, void bitmap );
//! Create a new GDK2.Bitmap object.
//! Argument is either an Image.image object, or {xsisze,ysize,xbitmapdata}.
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
