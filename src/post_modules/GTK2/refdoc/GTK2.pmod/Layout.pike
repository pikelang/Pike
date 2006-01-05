//! Properties:
//! GTK2.Adjustment hadjustment
//! int height
//! GTK2.Adjustment vadjustment
//! int width
//! 
//! Child properties:
//! int x
//! int y
//!
//!
//!  Signals:
//! @b{set_scroll_adjustments@}
//!

inherit GTK2.Container;

static GTK2.Layout create( GTK2.Adjustment hadjustment_or_props, GTK2.Adjustment vadjustment );
//! Creates a new GTK2.Layout.
//!
//!

GTK2.Adjustment get_hadjustment( );
//! Gets the GTK2.Adjustment used for communicaiton between
//! the horizontal scrollbar and this layout.  This should
//! only be called after the layout has been placed in a
//! GTK2.ScrolledWindow or otherwise configured for scrolling.
//!
//!

GTK2.Adjustment get_vadjustment( );
//! Gets the vertical GTK2.Adjustment.
//!
//!

GTK2.Layout move( GTK2.Widget widget, int x, int y );
//! Moves a current child to a new position.
//!
//!

GTK2.Layout put( GTK2.Widget widget, int x, int y );
//! Adds widget to the layout at position (x,y).
//! The layout becomes the new parent.
//!
//!

GTK2.Layout set_hadjustment( GTK2.Adjustment adj );
//! Sets the horizontal scroll adjustment for the layout.
//!
//!

GTK2.Layout set_size( int xsize, int ysize );
//! Sets the size of the scrollable area of the layout.
//!
//!

GTK2.Layout set_vadjustment( GTK2.Adjustment adj );
//! Sets the vertical scroll adjustment for the layout.
//!
//!
