//! A fixed container is a container that keeps it's children at fixed
//! locations and give them fixed sizes, both given in pixels.
//! 
//!  Example:
//!@code{ GTK.Fixed()->put(GTK.Label("100,100"), 100, 100)->put(GTK.Label("0,0"), 0, 0)->set_usize(150,115)@}
//!@xml{<image>../images/gtk_fixed.png</image>@}
//!
//! 
//!
//!

inherit GTK.Container;

static GTK.Fixed create( );
//! Create a new fixed widget
//!
//!

GTK.Fixed move( GTK.Widget widget, int new_xpos, int new_ypos );
//! Move the widget to new_xpos,new_ypos from it's old location.
//!
//!

GTK.Fixed put( GTK.Widget widget, int x, int y );
//! Place the widget at xpos,ypos.
//!
//!
