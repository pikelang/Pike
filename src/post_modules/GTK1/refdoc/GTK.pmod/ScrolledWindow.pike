//! Scrolled windows are used to create a scrollable area with another
//! widget inside it. You may insert any type of widget into a scrolled
//! window, and it will be accessible regardless of the size by using
//! the scrollbars.
//! 
//!@expr{ GTK.ScrolledWindow(GTK.Adjustment(),GTK.Adjustment())->add(GTK.Label("A small label"))->set_usize(100,80)->set_policy(GTK.POLICY_AUTOMATIC,GTK.POLICY_AUTOMATIC)@}
//!@xml{<image>../images/gtk_scrolledwindow.png</image>@}
//!
//!@expr{ GTK.ScrolledWindow(GTK.Adjustment(),GTK.Adjustment())->add(GTK.Label("A small label"))->set_usize(70,80)->set_policy(GTK.POLICY_AUTOMATIC,GTK.POLICY_AUTOMATIC)@}
//!@xml{<image>../images/gtk_scrolledwindow_2.png</image>@}
//!
//!@expr{ GTK.ScrolledWindow(GTK.Adjustment(),GTK.Adjustment())->add(GTK.Label("A small label"))->set_usize(80,80)@}
//!@xml{<image>../images/gtk_scrolledwindow_3.png</image>@}
//!
//!@expr{ GTK.ScrolledWindow(GTK.Adjustment(),GTK.Adjustment())->add(GTK.Label("A very huge label")->set_usize(700,700))->set_usize(80,80)@}
//!@xml{<image>../images/gtk_scrolledwindow_4.png</image>@}
//!
//! 
//!
//!

inherit GTK.Window;

GTK.ScrolledWindow add( GTK.Widget victim );
//! Add a widget to this container.
//! This is equivalent to the
//! C-GTK function gtk_scrolled_window_add_with_viewport or
//! gtk_container_add, depeneding on whether or not the child supports
//! the set_scroll_adjustments signal.
//! 
//! What this means in practice is that you do not have to care about
//! this at all, it's all handled automatically.
//! 
//!
//!

static GTK.ScrolledWindow create( GTK.Adjustment hadjustment, GTK.Adjustment vadjustments );
//! The two adjustments are most commonly set to 0.
//!
//!

GTK.Adjustment get_hadjustment( );
//! Return the horizontal adjustment used to scroll the window
//!
//!

GTK.Hscrollbar get_hscrollbar( );
//! The horizontal scrollbar
//!
//!

int get_hscrollbar_policy( );
//! One of @[POLICY_ALWAYS], @[POLICY_AUTOMATIC] and @[POLICY_NEVER]
//!
//!

int get_hscrollbar_visible( );
//! 1 if the horizontal scrollbar is currently visible
//!
//!

GTK.Adjustment get_vadjustment( );
//! Return the vertical adjustment used to scroll the window
//!
//!

GTK.Vscrollbar get_vscrollbar( );
//! The vertical scrollbar
//!
//!

int get_vscrollbar_policy( );
//! One of @[POLICY_ALWAYS], @[POLICY_AUTOMATIC] and @[POLICY_NEVER]
//!
//!

int get_vscrollbar_visible( );
//! 1 if the vertical scrollbar is currently visible
//!
//!

int get_window_placement( );
//! The location of the window relative to the scrollbars.
//! One of @[CORNER_BOTTOM_LEFT], @[CORNER_BOTTOM_RIGHT], @[CORNER_TOP_LEFT] and @[CORNER_TOP_RIGHT]
//!
//!

GTK.ScrolledWindow set_hadjustment( GTK.Adjustment hadjustment );
//! Set the horizontal adjustment object
//!
//!

GTK.ScrolledWindow set_placement( int window_corner_placement );
//! The location of the window relative to the scrollbars.
//! One of @[CORNER_BOTTOM_LEFT], @[CORNER_BOTTOM_RIGHT], @[CORNER_TOP_LEFT] and @[CORNER_TOP_RIGHT]
//!
//!

GTK.ScrolledWindow set_policy( int xpolicy, int ypolicy );
//! vertical and horiz policy. Both are one of @[POLICY_ALWAYS], @[POLICY_AUTOMATIC] and @[POLICY_NEVER]
//!
//!

GTK.ScrolledWindow set_vadjustment( GTK.Adjustment vadjustment );
//! Set the vertical adjustment object
//!
//!
