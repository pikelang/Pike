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

inherit GTK.Misc;

static GTK.Arrow create( int arrow_type, int shadow_type );
//! First argument is one of @[ARROW_DOWN], @[ARROW_UP], @[ARROW_RIGHT] and @[ARROW_LEFT], second one of @[SHADOW_OUT], @[SHADOW_ETCHED_IN], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_ETCHED_OUT].
//!
//!

int get_arrow_type( );
//! Return the arrow type. One of @[ARROW_DOWN], @[ARROW_UP], @[ARROW_RIGHT] and @[ARROW_LEFT].
//!
//!

int get_shadow_type( );
//! Return the arrow type. One of @[SHADOW_OUT], @[SHADOW_ETCHED_IN], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_ETCHED_OUT].
//!
//!

GTK.Arrow set( int arrow_type, int shadow_type );
//! First argument is one of @[ARROW_DOWN], @[ARROW_UP], @[ARROW_RIGHT] and @[ARROW_LEFT], second one of @[SHADOW_OUT], @[SHADOW_ETCHED_IN], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_ETCHED_OUT].
//!
//!
