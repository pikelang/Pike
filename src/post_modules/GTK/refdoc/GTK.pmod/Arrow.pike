//! An arrow pointing in one of four directions. The 'etched' shadow
//! types does not work.
//!@code{ GTK.Arrow(GTK.ARROW_UP, GTK.SHADOW_OUT)@}
//!@xml{<image>../images/gtk_arrow.png</image>@}
//!
//!@code{ GTK.Arrow(GTK.ARROW_LEFT, GTK.SHADOW_IN)@}
//!@xml{<image>../images/gtk_arrow_2.png</image>@}
//!
//!@code{ GTK.Arrow(GTK.ARROW_RIGHT, GTK.SHADOW_IN)@}
//!@xml{<image>../images/gtk_arrow_3.png</image>@}
//!
//!@code{ GTK.Arrow(GTK.ARROW_DOWN, GTK.SHADOW_OUT)@}
//!@xml{<image>../images/gtk_arrow_4.png</image>@}
//!
//!
//!

inherit GTK.Misc;

static GTK.Arrow create( int arrow_type, int shadow_type );
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

GTK.Arrow set( int arrow_type, int shadow_type );
//! First argument is one of @[ARROW_DOWN], @[ARROW_LEFT], @[ARROW_RIGHT] and @[ARROW_UP], second one of @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_OUT].
//!
//!
