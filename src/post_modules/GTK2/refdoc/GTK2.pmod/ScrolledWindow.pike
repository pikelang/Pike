//! Scrolled windows are used to create a scrollable area with another
//! widget insite it.  You may insert any type of widget into a scrolled
//! window, and it will be accessible regardless of the size by using
//! the scrollbars.
//!
//!@expr{ GTK2.ScrolledWindow(GTK2.Adjustment(),GTK2.Adjustment())->add(GTK2.Label("A small label"))->set_policy(GTK2.POLICY_AUTOMATIC,GTK2.POLICY_AUTOMATIC)@}
//!@xml{<image>../images/gtk2_scrolledwindow.png</image>@}
//!
//!@expr{ GTK2.ScrolledWindow(GTK2.Adjustment(),GTK2.Adjustment())->add(GTK2.Label("A small label"))->set_policy(GTK2.POLICY_AUTOMATIC,GTK2.POLICY_AUTOMATIC)@}
//!@xml{<image>../images/gtk2_scrolledwindow_2.png</image>@}
//!
//!@expr{ GTK2.ScrolledWindow(GTK2.Adjustment(),GTK2.Adjustment())->add(GTK2.Label("A small label"))@}
//!@xml{<image>../images/gtk2_scrolledwindow_3.png</image>@}
//!
//!@expr{ GTK2.ScrolledWindow(GTK2.Adjustment(),GTK2.Adjustment())->add(GTK2.Label("a ver huge label"))@}
//!@xml{<image>../images/gtk2_scrolledwindow_4.png</image>@}
//!
//! 
//! Properties:
//! GTK2.Adjustment hadjustment
//! int hscrollbar-policy
//! int shadow-type
//! GTK2.Adjustment vadjustment
//! int vscrollbar-policy
//! int window-placement
//! 
//! Style properties:
//! int scrollbar-spacing
//!
//!
//!  Signals:
//! @b{move_focus_out@}
//!
//! @b{scroll_child@}
//!

inherit GTK2.Bin;

GTK2.ScrolledWindow add( GTK2.Widget victim );
//! Add a widget to this container.
//! This is equivalent to the
//! C-GTK function gtk_scrolled_window_add_with_viewport or
//! gtk_container_add, depending on whether or not the child supports
//! the set_scroll_adjustments signal.
//! 
//! What this means in practice is that you do not have to care about
//! this at all, it's all handled automatically.
//! 
//!
//!

static GTK2.ScrolledWindow create( GTK2.Adjustment hadjustment_or_props, GTK2.Adjustment vadjustments );
//! The two adjustments are most commonly set to 0.
//!
//!

GTK2.Adjustment get_hadjustment( );
//! Get the horizontal adjustment.
//!
//!

GTK2.HScrollbar get_hscrollbar( );
//! Returns the horizontal scrollbar.
//!
//!

int get_placement( );
//! Gets the placement of the scrollbars.
//!
//!

mapping get_policy( );
//! Returns the current policy values for the horizontal and vertical
//! scrollbars.
//! ([ "h-policy": horizontal policy,
//!    "v-policy": vertical policy
//! ]);
//!
//!

int get_shadow_type( );
//! Gets the shadow type.
//!
//!

GTK2.Adjustment get_vadjustment( );
//! Get the vertical adjustment.
//!
//!

GTK2.VScrollbar get_vscrollbar( );
//! Returns the vertical scrollbar.
//!
//!

GTK2.ScrolledWindow set_hadjustment( GTK2.Adjustment hadjustment );
//! Set the horizontal adjustment object.
//!
//!

GTK2.ScrolledWindow set_placement( int window_placement );
//! The location of the window relative to the scrollbars.
//! One of @[CORNER_BOTTOM_LEFT], @[CORNER_BOTTOM_RIGHT], @[CORNER_TOP_LEFT] and @[CORNER_TOP_RIGHT]
//!
//!

GTK2.ScrolledWindow set_policy( int xpolicy, int ypolicy );
//! Vertical and horizontal policy.  Both are one of @[POLICY_ALWAYS], @[POLICY_AUTOMATIC] and @[POLICY_NEVER]
//!
//!

GTK2.ScrolledWindow set_shadow_type( int type );
//! Changes the type of shadow drawn around the contents.
//! One of @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_OUT]
//!
//!

GTK2.ScrolledWindow set_vadjustment( GTK2.Adjustment vadjustment );
//! Set the vertical adjustment object.
//!
//!
