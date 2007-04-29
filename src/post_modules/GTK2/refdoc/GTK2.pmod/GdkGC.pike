//! A GC, or Graphics Context, is used for most low-level drawing operations.
//! 
//! As an example, the foreground color, background color, and drawing
//! function is stored in the GC.
//!
//! NOIMG
//!
//!

inherit G.Object;

GDK2.GC copy( GTK2.GdkGC source );
//! Copy all attributes from the source GC
//!
//!

static GDK2.GC create( GTK2.Widget context, mapping|void attributes );
//! The argument is either a W(Widget) or a GDK2(Drawable) in
//! which the gc will be valid.
//!
//!


GTK2.GdkScreen get_screen( );
//! Gets the screen.
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

GDK2.GC set_clip_rectangle( GTK2.GdkRectangle rect );
//! Sets the clip mask for a graphics context from a rectangle.  The
//! clip mask is interpreted relative to the clip origin.
//!
//!

GDK2.GC set_clip_region( GTK2.GdkRegion rect );
//! Sets the clip mask for a graphs context from a region.  The
//! clip mask is interpreted relative to the clip origin.
//!
//!

GDK2.GC set_dashes( int offset, array dashes );
//! Sets the way dashed-lines are drawn.  Lines will be drawn with
//! alternating on and off segments of the lengths specified in
//! dashes.  The manner in which the on and off segments are drawn is
//! determined by the line_style value of the GC.
//!
//!

GDK2.GC set_exposures( int exp );
//! Sets whether copying non-visible portions of a drawable using this gc
//! generates exposure events for the corresponding regions of the dest
//! drawable.
//!
//!

GDK2.GC set_fill( int fill );
//! Set the fill method to fill.
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

GDK2.GC set_line_attributes( int line_width, int line_style, int cap_style, int join_style );
//! Control how lines are drawn.
//! line_style is one of GDK2.LineSolid, GDK2,LineOnOffDash and GDK2.LineDoubleDash.
//! cap_style is one of GDK2.CapNotLast, GDK2.CapButt, GDK2.CapRound and GDK2.CapProjecting.
//! join_style is one of GDK2.JoinMiter, GDK2.JoinRound and GDK2.JoinBevel.
//!
//!

GDK2.GC set_stipple( GTK2.GdkBitmap stipple );
//! Set the background type.  Fill must be GDK_STIPPLED or GDK_OPAQUE_STIPPLED.
//!
//!

GDK2.GC set_subwindow( int draw_on_subwindows );
//! If set, anything drawn with this GC will draw on subwindows as well
//! as the window in which the drawing is done.
//!
//!

GDK2.GC set_tile( GTK2.GdkPixmap tile );
//! Set the background type.  Fill must be GDK_TILED
//!
//!

GDK2.GC set_ts_origin( int x, int y );
//! Set the origin when using tiles or stipples with the GC.  The tile
//! or stipple will be aligned such that the upper left corner of the
//! tile or stipple will coincide with this point.
//!
//!
