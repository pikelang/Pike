//! The contents of a handle box can be 'dragged' out of the box by the user.
//! The contents will then be placed in a separate window.
//!@expr{ GTK.HandleBox()->add(GTK.Label("The contents"))@}
//!@xml{<image>../images/gtk_handlebox.png</image>@}
//!
//!
//!
//!  Signals:
//! @b{child_attached@}
//! Called when a new child is added to the box
//!
//!
//! @b{child_detached@}
//! Called when a child is removed from the box
//!
//!

inherit GTK.Bin;

static GTK.HandleBox create( );
//! Create a new handle box widget.
//!
//!

int get_child_detached( );
//! Is the child detatched?
//!
//!

int get_float_window_mapped( );
//! Is the floating window mapped?
//!
//!

int get_handle_position( );
//! The position of the handle. One of @[POS_BOTTOM], @[POS_LEFT], @[POS_RIGHT] and @[POS_TOP]
//!
//!

int get_in_drag( );
//! 1 if the window is beeing dragged around right now.
//!
//!

int get_shrink_on_detach( );
//! True if the container should shrink when the child is detatched
//!
//!

int get_snap_edge( );
//! The edge to snap to. One of @[POS_BOTTOM], @[POS_LEFT], @[POS_RIGHT] and @[POS_TOP], or -1 for unset.
//!
//!

GTK.HandleBox set_handle_position( int pos );
//! The position of the handle. One of @[POS_BOTTOM], @[POS_LEFT], @[POS_RIGHT] and @[POS_TOP]
//!
//!

int set_set_shrink_on_detach( );
//! True if the container should shrink when the child is detatched
//!
//!

GTK.HandleBox set_shadow_type( int shadow_type );
//! One of @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_OUT]
//!
//!

GTK.HandleBox set_snap_edge( int pos );
//! The edge to snap to. One of @[POS_BOTTOM], @[POS_LEFT], @[POS_RIGHT] and @[POS_TOP], or -1 for unset.
//!
//!
