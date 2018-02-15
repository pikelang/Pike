//! An arrow pointing in one of four directions. The 'etched' shadow
//! types does not work.
//!@expr{ GTK1.Arrow(GTK1.ARROW_UP, GTK1.SHADOW_OUT)@}
//!@xml{<image>../images/gtk1_arrow.png</image>@}
//!
//!@expr{ GTK1.Arrow(GTK1.ARROW_LEFT, GTK1.SHADOW_IN)@}
//!@xml{<image>../images/gtk1_arrow_2.png</image>@}
//!
//!@expr{ GTK1.Arrow(GTK1.ARROW_RIGHT, GTK1.SHADOW_IN)@}
//!@xml{<image>../images/gtk1_arrow_3.png</image>@}
//!
//!@expr{ GTK1.Arrow(GTK1.ARROW_DOWN, GTK1.SHADOW_OUT)@}
//!@xml{<image>../images/gtk1_arrow_4.png</image>@}
//!
//!
//!

inherit GTK1.Misc;

protected GTK1.Arrow create( int arrow_type, int shadow_type );
//! First argument is one of @[ARROW_DOWN], @[ARROW_LEFT], @[ARROW_RIGHT] and @[ARROW_UP], second one of @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_OUT].
//!
//!

int get_arrow_type( );
//! Return the arrow type. One of @[ARROW_DOWN], @[ARROW_LEFT], @[ARROW_RIGHT] and @[ARROW_UP].
//!
//!

int get_shadow_type( );
//! Return the shadow type. One of @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_OUT].
//!
//!

GTK1.Arrow set( int arrow_type, int shadow_type );
//! First argument is one of @[ARROW_DOWN], @[ARROW_LEFT], @[ARROW_RIGHT] and @[ARROW_UP], second one of @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_OUT].
//!
//!
