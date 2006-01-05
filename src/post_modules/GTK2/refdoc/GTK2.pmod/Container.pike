//! The basic container class.
//! Properties:
//! int border-width
//! GTK2.Widget child
//! int resize-mode @[RESIZE_IMMEDIATE], @[RESIZE_PARENT] and @[RESIZE_QUEUE]
//!
//!
//!  Signals:
//! @b{add@}
//! Called when a new object is added to the container. (used internally)
//!
//!
//! @b{check_resize@}
//! Called when the container needs resizing (used internally)
//!
//!
//! @b{remove@}
//! Called when a object is removed from the container (used internally)
//!
//!
//! @b{set_focus_child@}
//!

inherit GTK2.Widget;

GTK2.Container add( GTK2.Widget widget );
//! Add a subwidget to the container. Don't forget to call show() in
//! the subwidget. Some (even most) containers can only contain one child.
//! Calling this function might result in a resize of the container.
//!
//!

GTK2.Container check_resize( );
//! Undocumented
//!
//!

int get_border_width( );
//! Retrieves the border width.
//!
//!

array get_children( );
//! This function returns all children of the container
//! as an array.
//!
//!

array get_focus_chain( );
//! Returns the focus chain.
//!
//!

GTK2.Adjustment get_focus_hadjustment( );
//! Retrieves the horizontal focus adjustment.
//!
//!

GTK2.Adjustment get_focus_vadjustment( );
//! Retrieves the vertical focus adjustment.
//!
//!

int get_resize_mode( );
//! Returns the resize mode.
//!
//!

GTK2.Container remove( GTK2.Widget widget );
//! Remove a child from the container. The argument is the child to remove.
//! Calling this function might result in a resize of the container.
//!
//!

GTK2.Container resize_children( );
//! Undocumented.
//!
//!

GTK2.Container set_border_width( int width );
//! Sets the border width.
//!
//!

GTK2.Container set_focus_chain( array focusable_widgets );
//! Sets a focus chain, overriding the one computer automatically by GTK+.
//! 
//! In principle each widget in the chain should be a descendant of the
//! container, but this is not enforced by this method, since it's allowed to
//! set the focus chain before you pack the widgets, or have a widget in the
//! chain that isn't always packed.  The necessary checks are done when the
//! focus chain is actually traversed.
//!
//!

GTK2.Container set_focus_child( GTK2.Widget child );
//! Emulate a set_focus_child signal. Focus on the specified child.
//!
//!

GTK2.Container set_focus_hadjustment( GTK2.Adjustment adj );
//! Set the hadjustment used to focus children.
//!
//!

GTK2.Container set_focus_vadjustment( GTK2.Adjustment adj );
//! Set the vadjustment used to focus children.
//!
//!

GTK2.Container set_reallocate_redraws( int setting );
//! Sets the reallocate_redraws flag.
//!
//!

GTK2.Container set_resize_mode( int mode );
//! One of @[RESIZE_IMMEDIATE], @[RESIZE_PARENT] and @[RESIZE_QUEUE]
//!
//!
