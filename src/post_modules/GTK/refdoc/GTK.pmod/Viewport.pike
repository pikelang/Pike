//! This is a container that can be scrolled around, but it has no scrollbars.
//! You can connect scrollbars to it using the adjustment objects.
//!@code{ GTK.Viewport(GTK.Adjustment(),GTK.Adjustment())->set_usize(100,100)@}
//!@xml{<image>../images/gtk_viewport.png</image>@}
//!
//!@code{ GTK.Viewport(GTK.Adjustment(),GTK.Adjustment())->set_usize(100,100)->set_shadow_type(GTK.SHADOW_ETCHED_IN)@}
//!@xml{<image>../images/gtk_viewport_2.png</image>@}
//!
//!@code{ GTK.Viewport(GTK.Adjustment(),GTK.Adjustment())->set_usize(100,100)->add(GTK.Label("A label with a very long text on it, it will not fit"))->set_shadow_type(GTK.SHADOW_ETCHED_IN)@}
//!@xml{<image>../images/gtk_viewport_3.png</image>@}
//!
//!@code{ lambda(){ object a1;object v = GTK.Viewport(a1=GTK.Adjustment(),GTK.Adjustment())->set_usize(100,100)->add(GTK.Label("A label with a very long text on it, it will not fit"))->set_shadow_type(GTK.SHADOW_ETCHED_IN);call_out(a1->set_value,0,100.0);return v;}()@}
//!@xml{<image>../images/gtk_viewport_4.png</image>@}
//!
//!
//!

inherit GTK.Bin;

static GTK.Viewport create( GTK.Adjustment xscroll, GTK.Adjustment yscroll );
//! Create a new viewport.  The adjustments are used to select what
//! part of the viewport to view to the user. They are normally
//! connected to a scrollbar or something similar.
//!
//!

GTK.Adjustment get_hadjustment( );
//! Return the current horizontal adjustment object
//!
//!

GTK.Adjustment get_vadjustment( );
//! Return the current vertical adjustment object
//!
//!

GTK.Viewport set_hadjustment( GTK.Adjustment xscroll );
//! Set a new horizontal adjustment object.
//!
//!

GTK.Viewport set_shadow_type( int type );
//! Set the shadow style. One of @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_OUT]
//!
//!

GTK.Viewport set_vadjustment( GTK.Adjustment yscroll );
//! Set a new vertical adjustment object.
//!
//!
