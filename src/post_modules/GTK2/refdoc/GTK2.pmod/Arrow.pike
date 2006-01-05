//! An arrow pointing in one of four directions. The 'etched' shadow
//! types does not work.
//!@expr{ GTK2.Arrow(GTK2.ARROW_UP, GTK2.SHADOW_OUT)@}
//!@xml{<image>../images/gtk2_arrow.png</image>@}
//!
//!@expr{ GTK2.Arrow(GTK2.ARROW_LEFT, GTK2.SHADOW_IN)@}
//!@xml{<image>../images/gtk2_arrow_2.png</image>@}
//!
//!@expr{ GTK2.Arrow(GTK2.ARROW_RIGHT, GTK2.SHADOW_IN)@}
//!@xml{<image>../images/gtk2_arrow_3.png</image>@}
//!
//!@expr{ GTK2.Arrow(GTK2.ARROW_DOWN, GTK2.SHADOW_OUT)@}
//!@xml{<image>../images/gtk2_arrow_4.png</image>@}
//!
//! Properties:
//! int arrow-type @[ARROW_DOWN], @[ARROW_LEFT], @[ARROW_RIGHT] and @[ARROW_UP]
//! int shadow-type @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_OUT]
//!
//!

inherit GTK2.Misc;

static GTK2.Arrow create( int|mapping arrow_type_or_props, int|void shadow_type );
//! First argument is one of @[ARROW_DOWN], @[ARROW_LEFT], @[ARROW_RIGHT] and @[ARROW_UP], second one of @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_OUT].
//!
//!

GTK2.Arrow set( int arrow_type, int shadow_type );
//! First argument is one of @[ARROW_DOWN], @[ARROW_LEFT], @[ARROW_RIGHT] and @[ARROW_UP], second one of @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_OUT].
//!
//!
