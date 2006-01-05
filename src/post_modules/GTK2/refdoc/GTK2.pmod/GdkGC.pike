//! A GC, or Graphics Context, is used for most low-level drawing operations.
//! 
//! As an example, the foreground color, background color, and drawing
//! function is stored in the GC.
//!
//! NOIMG
//!
//!

inherit G.Object;

static GDK2.GC create( GTK2.Widget context );
//! The argument is either a W(Widget) or a GDK2(Drawable) in
//! which the gc will be valid.
//!
//!

GDK2.GC destroy( );
//! Free the gc, called automatically by pike when the object is destroyed.
//!
//!

mapping get_values( );
//! Get all (or rather most) values from the GC.
//! Even though GdkGCValues contains a GdkFont object, we won't return
//! this value because GdkFont is deprecated.  The Pango methods should
//! be used instead.
//!
//!

GDK2.GC set_background( GTK2.GdkColor color );
//! Set the background to the specified GDK2.Color.
//!
//!

GDK2.GC set_clip_mask( GTK2.GdkBitmap mask );
//! Set the clip mask to the specified GDK2.Bitmap
//!
//!

GDK2.GC set_clip_origin( int x, int y );
//! Set the clip mask origin to the specified point.
//!
//!

GDK2.GC set_foreground( GTK2.GdkColor color );
//! Set the foreground to the specified GDK2.Color.
//!
//!

GDK2.GC set_function( int fun );
//! Set the function to the specified one.  One of GDK2.Xor,
//! GDK2.Invert and GDK2.Copy.
//!
//!

GDK2.GC set_subwindow( int draw_on_subwindows );
//! If set, anything drawn with this GC will draw on subwindows as well
//! as the window in which the drawing is done.
//!
//!
