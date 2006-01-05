//! This is a container that can be scrolled around, but it has no scrollbars.
//! You can connect scrollbars to it using the adjustment objects.
//!@expr{ GTK2.Viewport(GTK2.Adjustment(),GTK2.Adjustment())->set_size_request(100,100)@}
//!@xml{<image>../images/gtk2_viewport.png</image>@}
//!
//!@expr{ GTK2.Viewport(GTK2.Adjustment(),GTK2.Adjustment())->set_size_request(100,100)->set_shadow_type(GTK2.SHADOW_ETCHED_IN)@}
//!@xml{<image>../images/gtk2_viewport_2.png</image>@}
//!
//!@expr{ GTK2.Viewport(GTK2.Adjustment(),GTK2.Adjustment())->set_size_request(100,100)->add(GTK2.Label("A label with a very long text on it, it will not fit"))->set_shadow_type(GTK2.SHADOW_ETCHED_IN)@}
//!@xml{<image>../images/gtk2_viewport_3.png</image>@}
//!
//!@expr{ lambda(){ object a1;object v = GTK2.Viewport(a1=GTK2.Adjustment(),GTK2.Adjustment())->set_size_request(100,100)->add(GTK2.Label("A label with a very long text on it, it will not fit"))->set_shadow_type(GTK2.SHADOW_ETCHED_IN);call_out(a1->set_value,0,100.0);return v;}()@}
//!@xml{<image>../images/gtk2_viewport_4.png</image>@}
//!
//! Properties:
//! GTK2.Adjustment hadjustment
//! int shadow-type
//! GTK2.Adjustment vadjustment
//!
//!
//!  Signals:
//! @b{set_scroll_adjustments@}
//!

inherit GTK2.Bin;

static GTK2.Viewport create( GTK2.Adjustment xscroll_or_props, GTK2.Adjustment yscroll );
//! Create a new viewport.  The adjustments are used to select what
//! part of the viewport to view to the user. They are normally
//! connected to a scrollbar or something similar.
//!
//!

GTK2.Adjustment get_hadjustment( );
//! Return the current horizontal adjustment object
//!
//!

int get_shadow_type( );
//! Get the shadow type.  One Of @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_OUT]
//!
//!

GTK2.Adjustment get_vadjustment( );
//! Return the current vertical adjustment object
//!
//!

GTK2.Viewport set_hadjustment( GTK2.Adjustment xscroll );
//! Set a new horizontal adjustment object.
//!
//!

GTK2.Viewport set_shadow_type( int type );
//! Set the shadow style. One of @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_OUT]
//!
//!

GTK2.Viewport set_vadjustment( GTK2.Adjustment yscroll );
//! Set a new vertical adjustment object.
//!
//!
