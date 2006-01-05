//! GTK2.Paned is the base class for widgets with two panes, arranged
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
//! with the shadow type set to GTK2.ShadowIn so that the gutter
//! appears as a ridge.
//! 
//! Each child has two options that can be set, resize and shrink. If
//! resize is true, then when the GTK2.Paned is resized, that child will
//! expand or shrink along with the paned widget. If shrink is true,
//! then when that child can be made smaller than it's requisition by
//! the user. Setting shrink to 0 allows the application to set a
//! minimum size. If resize is false for both children, then this is
//! treated as if resize is true for both children.
//! 
//! The application can set the position of the slider as if it were
//! set by the user, by calling set_position().
//! 
//! Properties:
//! int position
//! int position-set
//! 
//! 
//! Style properties:
//! int handle-size
//!
//!
//!  Signals:
//! @b{accept_position@}
//!
//! @b{cancel_position@}
//!
//! @b{cycle_child_focus@}
//!
//! @b{cycle_handle_focus@}
//!
//! @b{move_handle@}
//!
//! @b{toggle_handle_focus@}
//!

inherit GTK2.Container;

GTK2.Paned add1( GTK2.Widget left_or_top );
//! Set the left or topmost item.
//! This is equivalent to pack1(left_or_top,0,1)
//!
//!

GTK2.Paned add2( GTK2.Widget right_or_bottom );
//! Set the right or bottommost item
//! This is equivalent to pack2(left_or_top,0,1)
//!
//!

int get_position( );
//! Obtains the position of the divider between the two panes.
//!
//!

GTK2.Paned pack1( GTK2.Widget widget, int resize, int shrink );
//! Add a child to the top or left pane.
//!
//!

GTK2.Paned pack2( GTK2.Widget widget, int resize, int shrink );
//! Add a child to the bottom or right pane.
//!
//!

GTK2.Paned set_position( int position );
//! Set the position of the separator, as if set by the user. If
//! position is negative, the remembered position is forgotten, and
//! the division is recomputed from the the requisitions of the
//! children.
//!
//!
