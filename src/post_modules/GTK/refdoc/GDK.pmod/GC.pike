//! A GC, or Graphics Context, is used for most low-level drawing operation.
//! 
//! As an example, the foreground color, background color and drawing
//! function is stored in the GC.
//!
//! NOIMG
//!
//!

static GDK.GC create( GTK.Widget context );
//! The argument is a either a W(Widget) or a GDK(Drawable) in
//! which the gc will be valid.
//!
//!

GDK.GC destroy( );
//! Free the gc, called automatically by pike when the object is destroyed.
//!
//!

mapping get_values( );
//! Get all (or rather most) values from the GC.
//!
//!

GDK.GC set_background( GDK.Color color );
//! Set the background to the specified GDK.Color.
//!
//!

GDK.GC set_clip_mask( GDK.Bitmap mask );
//! Set the clip mask to the specified GDK.Bitmap
//!
//!

GDK.GC set_clip_origin( int x, int y );
//! Set the clip mask origin to the specified point.
//!
//!

GDK.GC set_font( GDK.Font font );
//! Set the font to the specified GDK.Font.
//!
//!

GDK.GC set_foreground( GDK.Color color );
//! Set the foreground to the specified GDK.Color.
//!
//!

GDK.GC set_function( int fun );
//! Set the function to the specified one. One of GDK.Xor,
//! GDK.Invert and GDK.Copy.
//!
//!

GDK.GC set_subwindow( int draw_on_subwindows );
//! If set, anything drawn with this GC will draw on subwindows as well
//! as the window in which the drawing is done.
//!
//!
