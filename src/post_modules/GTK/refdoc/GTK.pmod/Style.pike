//!

Style apply_default_background( GDK.Window window, int set_bgp, int state_type, GDK.Rectangle area, int x, int y, int width, int height );
//! Set the background of the specified window (or the subarea
//! indicated by the rectangle) to the default background for the state
//! specified by state_type.
//! 
//! If set_bgp is true, the background of the widget will be set,
//! otherwise it will only be drawn into the window.
//!
//!

GTK.Style attach( GDK.Window to );
//!   Attach a style to a window; this process allocates the colors and
//!   creates the GC's for the style - it specializes it to a
//!   particular visual and colormap. The process may involve the
//!   creation of a new style if the style has already been attached to
//!   a window with a different style and colormap.
//!
//!

GTK.Style copy( );
//!  Copy this style, and return the new style object
//!
//!

Style destroy( );
//!

Style detach( );
//!  Undo a previous attach
//!
//!

array(object(implements 1004)) get_base( );
//!

array(object(implements 1005)) get_base_gc( );
//!

array(object(implements 1004)) get_bg( );
//!

array(object(implements 1005)) get_bg_gc( );
//!

array(object(implements 1006)) get_bg_pixmap( );
//!

GDK.Color get_black( );
//!

GDK.GC get_black_gc( );
//!

array(object(implements 1004)) get_dark( );
//!

array(object(implements 1005)) get_dark_gc( );
//!

array(object(implements 1004)) get_fg( );
//!

array(object(implements 1005)) get_fg_gc( );
//!

GDK.Font get_font( );
//!

array(object(implements 1004)) get_light( );
//!

array(object(implements 1005)) get_light_gc( );
//!

array(object(implements 1004)) get_mid( );
//!

array(object(implements 1005)) get_mid_gc( );
//!

array(object(implements 1004)) get_text( );
//!

array(object(implements 1005)) get_text_gc( );
//!

GDK.Color get_white( );
//!

GDK.GC get_white_gc( );
//!
