//! A fixed container is a container that keeps it's children at fixed
//! locations and give them fixed sizes, both given in pixels.
//! 
//!  Example:
//!@expr{ GTK2.Fixed()->put(GTK2.Label("100,100"), 100, 100)->put(GTK2.Label("0,0"), 0, 0)->set_size_request(150,115)@}
//!@xml{<image>../images/gtk2_fixed.png</image>@}
//!
//! 
//! Child properties:
//! int x
//! int y
//!
//!

inherit GTK2.Container;

static GTK2.Fixed create( mapping|void props );
//! Create a new fixed widget
//!
//!

array get_children( );
//! Get a list of the children and their x,y positions.
//!
//!

int get_has_window( );
//! Gets whether GTK2.Fixed has its own GDK2.Window.
//!
//!

GTK2.Fixed move( GTK2.Widget widget, int new_xpos, int new_ypos );
//! Move the widget to new_xpos,new_ypos from it's old location.
//!
//!

GTK2.Fixed put( GTK2.Widget widget, int x, int y );
//! Place the widget at xpos,ypos.
//!
//!

GTK2.Fixed set_has_window( int has_window );
//! Sets wither a GTK2.Fixed widget is created with a separate
//! GDK2.Window for the window or now.
//!
//!
