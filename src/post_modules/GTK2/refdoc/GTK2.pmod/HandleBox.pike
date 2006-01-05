//! The contents of a handle box can be 'dragged' out of the box by the user.
//! The contents will then be placed in a separate window.
//!@expr{ GTK2.HandleBox()->add(GTK2.Label("The contents"))@}
//!@xml{<image>../images/gtk2_handlebox.png</image>@}
//!
//! Properties:
//! int handle-position
//! int shadow
//! int shadow-type
//! int snap-edge
//! int snap-edge-set
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

inherit GTK2.Bin;

static GTK2.HandleBox create( mapping|void props );
//! Create a new handle box widget.
//!
//!

int get_handle_position( );
//! Get the handle position.
//!
//!

int get_shadow_type( );
//! Get the shadow type.
//!
//!

int get_snap_edge( );
//! Get the snap edge.
//!
//!

GTK2.HandleBox set_handle_position( int pos );
//! The position of the handle. One of @[POS_BOTTOM], @[POS_LEFT], @[POS_RIGHT] and @[POS_TOP]
//!
//!

GTK2.HandleBox set_shadow_type( int shadow_type );
//! One of @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_OUT]
//!
//!

GTK2.HandleBox set_snap_edge( int pos );
//! The edge to snap to. One of @[POS_BOTTOM], @[POS_LEFT], @[POS_RIGHT] and @[POS_TOP], or -1 for unset.
//!
//!
