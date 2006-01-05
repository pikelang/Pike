//! Style settings.
//!
//!
//!  Signals:
//! @b{realize@}
//!
//! @b{unrealize@}
//!

inherit G.Object;

GTK2.Style attach( GTK2.Widget window );
//! Attaches a style to a window; this process allocates the colors and creates
//! the GC's for the style - it specializes it to a particular visual and
//! colormap.  The process may involve the creation of a new style if the style
//! has already been attached to a window with a different style and colormap.
//!
//!

GTK2.Style copy( );
//! Copy a style.
//!
//!

static GTK2.Style create( );
//! Creates a new style.
//!
//!

GTK2.Style detach( );
//! Detach a style.
//!
//!

GTK2.IconSet lookup_icon_set( string stock_id );
//! Lookup the icon set.
//!
//!

GTK2.Style paint_arrow( GTK2.Widget window, int state, int shadow, GTK2.GdkRectangle rect, GTK2.Widget widget, string detail, int arrow, int fill, int x, int y, int width, int height );
//! Draws an arrow in the given rectangle on window using the given parameters.
//! arrow determines the direction of the arrow.
//!
//!

GTK2.Style paint_box( GTK2.Widget window, int state, int shadow, GTK2.GdkRectangle rect, GTK2.Widget widget, string detail, int x, int y, int width, int height );
//! Draws a box on window with the given parameters.
//!
//!

GTK2.Style paint_box_gap( GTK2.Widget window, int state, int shadow, GTK2.GdkRectangle rect, GTK2.Widget widget, string detail, int x, int y, int width, int height, int gap_side, int gap_x, int gap_width );
//! Draws a box in window using the given style and state and shadow type,
//! leaving a gap in one side.
//!
//!

GTK2.Style paint_check( GTK2.Widget window, int state, int shadow, GTK2.GdkRectangle rect, GTK2.Widget widget, string detail, int x, int y, int width, int height );
//! Draws a check button indicator in the given rectangle on window.
//!
//!

GTK2.Style paint_diamond( GTK2.Widget window, int state, int shadow, GTK2.GdkRectangle rect, GTK2.Widget widget, string detail, int x, int y, int width, int height );
//! Draws a diamond in the given rectangle on window.
//!
//!

GTK2.Style paint_expander( GTK2.Widget window, int state, GTK2.GdkRectangle rect, GTK2.Widget widget, string detail, int x, int y, int style );
//! Draws an expander as used in GTK2.TreeView.  x and y specify the center of
//! the expander.  The size of the expander is determined by the
//! "expander-size" style property of widget.  (If widget is 0 or doesn't have
//! an "expander-size" property, an unspecified default size will be used,
//! since the caller doesn't have sufficient information to position the
//! expander, this is likely not useful.)  The expander is expander_size pixels
//! tall in the collapsed position and expander_size pixels wide in the
//! expanded position.
//!
//!

GTK2.Style paint_extension( GTK2.Widget window, int state, int shadow, GTK2.GdkRectangle rect, GTK2.Widget widget, string detail, int x, int y, int width, int height, int gap_side );
//! Draws an extension, i.e. a notebook tab.
//!
//!

GTK2.Style paint_flat_box( GTK2.Widget window, int state, int shadow, GTK2.GdkRectangle rect, GTK2.Widget widget, string detail, int x, int y, int width, int height );
//! Draws a flat box on window.
//!
//!

GTK2.Style paint_focus( GTK2.Widget window, int state, GTK2.GdkRectangle rect, GTK2.Widget widget, string detail, int x, int y, int width, int height );
//! Draws a focus indicator around the given rectangle.
//!
//!

GTK2.Style paint_handle( GTK2.Widget window, int state, int shadow, GTK2.GdkRectangle rect, GTK2.Widget widget, string detail, int x, int y, int width, int height, int orientation );
//! Draws a handle as used in GTK2.HandleBox and GTK2.Paned.
//!
//!

GTK2.Style paint_hline( GTK2.Widget window, int state, GTK2.GdkRectangle rect, GTK2.Widget widget, string detail, int x1, int x2, int y );
//! Draws a horizontal line from (x1,y) to (x2,y).
//!
//!

GTK2.Style paint_layout( GTK2.Widget window, int state, int use_text, GTK2.GdkRectangle rect, GTK2.Widget widget, string detail, int x, int y, GTK2.Pango.Layout layout );
//! Draw a pango layout.  Use widget->create_pango_layout() to get a
//! Pango.Layout.
//!
//!

GTK2.Style paint_option( GTK2.Widget window, int state, int shadow, GTK2.GdkRectangle rect, GTK2.Widget widget, string detail, int x, int y, int width, int height );
//! Draws a radio button indicator in the given rectangle.
//!
//!

GTK2.Style paint_resize_grip( GTK2.Widget window, int state, GTK2.GdkRectangle rect, GTK2.Widget widget, string detail, int edge, int x, int y, int width, int height );
//! Draws a resize grip in the given rectangle.
//!
//!

GTK2.Style paint_shadow( GTK2.Widget window, int state, int shadow, GTK2.GdkRectangle rect, GTK2.Widget widget, string detail, int x, int y, int width, int height );
//! Draws a shadow around the given rectangle.
//!
//!

GTK2.Style paint_shadow_gap( GTK2.Widget window, int state, int shadow, GTK2.GdkRectangle rect, GTK2.Widget widget, string detail, int x, int y, int width, int height, int gap_side, int gap_x, int gap_width );
//! Draws a shadow around the given rectangle in window, leaving a gap in one
//! side.
//!
//!

GTK2.Style paint_slider( GTK2.Widget window, int state, int shadow, GTK2.GdkRectangle rect, GTK2.Widget widget, string detail, int x, int y, int width, int height, int orientation );
//! Paint a slider.
//!
//!

GTK2.Style paint_tab( GTK2.Widget window, int state, int shadow, GTK2.GdkRectangle rect, GTK2.Widget widget, string detail, int x, int y, int width, int height );
//! Draws an option menu tab (i.e. the up and down pointing arrows).
//!
//!

GTK2.Style paint_vline( GTK2.Widget window, int state, GTK2.GdkRectangle rect, GTK2.Widget widget, string detail, int y1, int y2, int x );
//! Draws a vertical line from (x,y1) to (x,y2).
//!
//!

GTK2.GdkPixbuf render_icon( GTK2.IconSource source, int direction, int state, int size, GTK2.Widget widget, string detail );
//! Renders the icon specified by source at the given size according to the
//! given parameters and returns the result in a pixbuf.
//!
//!

GTK2.Style set_background( GTK2.Widget window, int state );
//! Sets the background of window to the background color or pixmap specified
//! by style for the given state.
//!
//!
