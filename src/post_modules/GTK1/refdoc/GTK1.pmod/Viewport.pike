//! This is a container that can be scrolled around, but it has no scrollbars.
//! You can connect scrollbars to it using the adjustment objects.
//!@expr{ GTK1.Viewport(GTK1.Adjustment(),GTK1.Adjustment())->set_usize(100,100)@}
//!@xml{<image>../images/gtk1_viewport.png</image>@}
//!
//!@expr{ GTK1.Viewport(GTK1.Adjustment(),GTK1.Adjustment())->set_usize(100,100)->set_shadow_type(GTK1.SHADOW_ETCHED_IN)@}
//!@xml{<image>../images/gtk1_viewport_2.png</image>@}
//!
//!@expr{ GTK1.Viewport(GTK1.Adjustment(),GTK1.Adjustment())->set_usize(100,100)->add(GTK1.Label("A label with a very long text on it, it will not fit"))->set_shadow_type(GTK1.SHADOW_ETCHED_IN)@}
//!@xml{<image>../images/gtk1_viewport_3.png</image>@}
//!
//!@expr{ lambda(){ object a1;object v = GTK1.Viewport(a1=GTK1.Adjustment(),GTK1.Adjustment())->set_usize(100,100)->add(GTK1.Label("A label with a very long text on it, it will not fit"))->set_shadow_type(GTK1.SHADOW_ETCHED_IN);call_out(a1->set_value,0,100.0);return v;}()@}
//!@xml{<image>../images/gtk1_viewport_4.png</image>@}
//!
//!
//!

inherit GTK1.Bin;

protected GTK1.Viewport create( GTK1.Adjustment xscroll, GTK1.Adjustment yscroll );
//! Create a new viewport.  The adjustments are used to select what
//! part of the viewport to view to the user. They are normally
//! connected to a scrollbar or something similar.
//!
//!

GTK1.Adjustment get_hadjustment( );
//! Return the current horizontal adjustment object
//!
//!

GTK1.Adjustment get_vadjustment( );
//! Return the current vertical adjustment object
//!
//!

GTK1.Viewport set_hadjustment( GTK1.Adjustment xscroll );
//! Set a new horizontal adjustment object.
//!
//!

GTK1.Viewport set_shadow_type( int type );
//! Set the shadow style. One of @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_OUT]
//!
//!

GTK1.Viewport set_vadjustment( GTK1.Adjustment yscroll );
//! Set a new vertical adjustment object.
//!
//!
