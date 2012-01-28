//! A GC, or Graphics Context, is used for most low-level drawing operation.
//! 
//! As an example, the foreground color, background color and drawing
//! function is stored in the GC.
//!
//! NOIMG
//!
//!

GDK1.GC copy( GDK1.GC source );
//! Copy all attributes from the source GC
//!
//!

protected GDK1.GC create( GTK1.Widget context, mapping|void attributes );
//! The argument is a either a W(Widget) or a GDK(Drawable) in
//! which the gc will be valid.
//!
//!

GDK1.GC destroy( );
//! Free the gc, called automatically by pike when the object is destroyed.
//!
//!

mapping get_values( );
//! Get all (or rather most) values from the GC.
//!
//!

GDK1.GC set_background( GDK1.Color color );
//! Set the background to the specified GDK1.Color.
//!
//!

GDK1.GC set_clip_mask( GDK1.Bitmap mask );
//! Set the clip mask to the specified GDK1.Bitmap
//!
//!

GDK1.GC set_clip_origin( int x, int y );
//! Set the clip mask origin to the specified point.
//!
//!

GDK1.GC set_clip_rectangle( GDK1.Rectangle rect );
//! Sets the clip mask for a graphics context from a rectangle. The
//! clip mask is interpreted relative to the clip origin.
//!
//!

GDK1.GC set_clip_region( GDK1.Region rect );
//! Sets the clip mask for a graphics context from a region. The
//! clip mask is interpreted relative to the clip origin.
//!
//!

GDK1.GC set_dashes( int offset, array dashes );
//! Sets the way dashed-lines are drawn. Lines will be drawn with
//! alternating on and off segments of the lengths specified in
//! dashes. The manner in which the on and off segments are drawn is
//! determined by the line_style value of the GC.
//!
//!

GDK1.GC set_fill( int fill );
//! Set the fill method to fill.
//!
//!

GDK1.GC set_font( GDK1.Font font );
//! Set the font to the specified GDK1.Font.
//!
//!

GDK1.GC set_foreground( GDK1.Color color );
//! Set the foreground to the specified GDK1.Color.
//!
//!

GDK1.GC set_function( int fun );
//! Set the function to the specified one. One of GDK1.Xor,
//! GDK1.Invert and GDK1.Copy.
//!
//!

GDK1.GC set_line_attributes( int line_width, int line_style, int cap_style, int join_style );
//! Control how lines are drawn.
//! line_style is one of GDK1.LineSolid, GDK1.LineOnOffDash and GDK1.LineDoubleDash.
//! cap_style is one of GDK1.CapNotLast, GDK1.CapButt, GDK1.CapRound and GDK1.CapProjecting.
//! join_style is one of GDK1.JoinMiter, GDK1.JoinRonud and GDK1.JoinBevel.
//!
//!

GDK1.GC set_stipple( GDK1.Bitmap stipple );
//! Set the background type. Fill must be GDK_STIPPLED or GDK_OPAQUE_STIPPLED
//!
//!

GDK1.GC set_subwindow( int draw_on_subwindows );
//! If set, anything drawn with this GC will draw on subwindows as well
//! as the window in which the drawing is done.
//!
//!

GDK1.GC set_tile( GDK1.Pixmap tile );
//! Set the background type. Fill must be GDK_TILED
//!
//!

GDK1.GC set_ts_origin( int x, int y );
//! Set the origin when using tiles or stipples with the GC. The tile
//! or stipple will be aligned such that the upper left corner of the
//! tile or stipple will coincide with this point.
//!
//!
