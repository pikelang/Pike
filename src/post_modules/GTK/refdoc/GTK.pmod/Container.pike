//! The basic container class.
//!
//!
//!  Signals:
//! @b{add@}
//! Called when a new object is added to the container. (used internally)
//!
//!
//! @b{focus@}
//! Called when the container gets focus (used internally)
//!
//!
//! @b{need_resize@}
//! Called when the container needs resizing (used internally)
//!
//!
//! @b{remove@}
//! Called when a object is removed from the container (used internally)
//!
//!
//! @b{set_focus_child@}
//!
inherit Widget;

Container add( GTK.Widget widget )
//! Add a subwidget to the container. Don't forget to call show() in
//! the subwidget. Some (even most) containers can only contain one child.
//! Calling this function might result in a resize of the container.
//!
//!

Container border_width( int width )
//! Compatibility alias for set_border_width.
//! Do not use this function in new code!
//!
//!

array(object(implements 1002)) children( )
//! This function returns all children of the container
//! as an array.
//!
//!

Container focus( int focus_direction )
//! Emulate a focus event. direction is one of @[DIR_RIGHT], @[DIR_TAB_BACKWARD], @[DIR_LEFT], @[DIR_DOWN], @[DIR_UP] and @[DIR_TAB_FORWARD].
//!
//!

Container remove( GTK.Widget widget )
//! Remove a child from the container. The argument is the child to remove.
//! Calling this function might result in a resize of the container.
//!
//!

Container set_border_width( int external_border_width )
//! Set the border width. The border width is the size of the
//! padding around the container.
//! Calling this function might result in a resize of the container.
//!
//!

Container set_focus_child( GTK.Widget child )
//! Emulate a set_focus_child signal. Focus on the specified child.
//!
//!

Container set_focus_hadjustment( GTK.Adjustment adj )
//! Set the hadjustment used to focus children.
//!
//!

Container set_focus_vadjustment( GTK.Adjustment adj )
//! Set the vadjustment used to focus children.
//!
//!

Container set_resize_mode( int mode )
//! One of @[RESIZE_PARENT], @[RESIZE_IMMEDIATE] and @[RESIZE_QUEUE]
//!
//!
