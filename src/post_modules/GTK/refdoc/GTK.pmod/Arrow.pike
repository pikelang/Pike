//! An arrow pointing in one of four directions. The 'etched' shadow
//! types does not work.
//!@code{ GTK.Arrow(GTK.ARROW_UP, GTK.SHADOW_OUT)@}
//!@xml{<image src='../images/gtk_arrow.png'/>@}
//!
//!@code{ GTK.Arrow(GTK.ARROW_LEFT, GTK.SHADOW_IN)@}
//!@xml{<image src='../images/gtk_arrow_2.png'/>@}
//!
//!@code{ GTK.Arrow(GTK.ARROW_RIGHT, GTK.SHADOW_IN)@}
//!@xml{<image src='../images/gtk_arrow_3.png'/>@}
//!
//!@code{ GTK.Arrow(GTK.ARROW_DOWN, GTK.SHADOW_OUT)@}
//!@xml{<image src='../images/gtk_arrow_4.png'/>@}
//!
//!
//!

inherit Misc;

static Arrow create( int arrow_type, int shadow_type );
//! First argument is one of @[ARROW_LEFT], @[ARROW_DOWN], @[ARROW_UP] and @[ARROW_RIGHT], second one of @[SHADOW_NONE], @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_OUT] and @[SHADOW_IN].
//!
//!

int get_arrow_type( );
//! Return the arrow type. One of @[ARROW_LEFT], @[ARROW_DOWN], @[ARROW_UP] and @[ARROW_RIGHT].
//!
//!

int get_shadow_type( );
//! Return the arrow type. One of @[SHADOW_NONE], @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_OUT] and @[SHADOW_IN].
//!
//!

Arrow set( int arrow_type, int shadow_type );
//! First argument is one of @[ARROW_LEFT], @[ARROW_DOWN], @[ARROW_UP] and @[ARROW_RIGHT], second one of @[SHADOW_NONE], @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_OUT] and @[SHADOW_IN].
//!
//!
