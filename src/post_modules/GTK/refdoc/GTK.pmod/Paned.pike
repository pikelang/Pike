//! GTK.Paned is the base class for widgets with two panes, arranged
//! either horizontally (W(HPaned)) or vertically (W(VPaned)). Child
//! widgets are added to the panes of the widget with pack1() and
//! pack2(). The division beween the two children is set by default
//! from the size requests of the children, but it can be adjusted by
//! the user.
//! 
//! A paned widget draws a separator between the two child widgets and
//! a small handle that the user can drag to adjust the division. It
//! does not draw any relief around the children or around the
//! separator. (The space in which the separator is called the
//! gutter). Often, it is useful to put each child inside a W(Frame)
//! with the shadow type set to GTK.ShadowIn so that the gutter
//! appears as a ridge.
//! 
//! Each child has two options that can be set, resize and shrink. If
//! resize is true, then when the GTK.Paned is resized, that child will
//! expand or shrink along with the paned widget. If shrink is true,
//! then when that child can be made smaller than it's requisition by
//! the user. Setting shrink to 0 allows the application to set a
//! minimum size. If resize is false for both children, then this is
//! treated as if resize is true for both children.
//! 
//! The application can set the position of the slider as if it were
//! set by the user, by calling set_position().
//! 
//!
//!

inherit GTK.Container;

GTK.Paned add1( GTK.Widget left_or_top );
//! Set the left or topmost item.
//! This is equivalent to pack1(left_or_top,0,1)
//!
//!

GTK.Paned add2( GTK.Widget right_or_bottom );
//! Set the right or bottommost item
//! This is equivalent to pack2(left_or_top,0,1)
//!
//!

int get_handle_size( );
//! The size of the handle, in pixels
//!
//!

int get_handle_xpos( );
//! The xpos of the handle, in pixels
//!
//!

int get_handle_ypos( );
//! The ypos of the handle, in pixels
//!
//!

int get_in_drag( );
//! Return 1 if the user is dragging the handle
//!
//!

int get_max_position( );
//! The maximum handle position possible.
//!
//!

int get_min_position( );
//! The minimum handle position possible.
//!
//!

GTK.Paned pack1( GTK.Widget widget, int resize, int shrink );
//! Add a child to the top or left pane.
//!
//!

GTK.Paned pack2( GTK.Widget widget, int resize, int shrink );
//! Add a child to the bottom or right pane.
//!
//!

GTK.Paned set_gutter_size( int gsize );
//! Set the width of the gutter. (The area between the two panes).
//!
//!

GTK.Paned set_handle_size( int hsize );
//! The size of the handle in pixels
//!
//!

GTK.Paned set_position( int position );
//! Set the position of the separator, as if set by the user. If
//! position is negative, the remembered position is forgotten, and
//! the division is recomputed from the the requisitions of the
//! children.
//!
//!
