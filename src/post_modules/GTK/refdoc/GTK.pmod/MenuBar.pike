//! Basically a horizontal W(Menu).
//! The menu image cannot be
//! grabbed automatically, but this is how you would create a menu all
//! in one line. This is not the recommended coding style.
//!@code{ GTK.MenuBar()->add(GTK.Menu_item("Menu")->set_submenu(GTK.Menu()->add(GTK.Menu_item("Sub")))->select()->activate())->add(GTK.Menu_item("Bar"))@}
//!@xml{<image>../images/gtk_menubar.png</image>@}
//!
//! 
//!
//!

inherit GTK.MenuShell;

GTK.MenuBar append( GTK.Widget menu );
//!

static GTK.MenuBar create( );
//!

GTK.MenuBar insert( GTK.Widget menu, int position );
//!

GTK.MenuBar prepend( GTK.Widget menu );
//!

GTK.MenuBar set_shadow_type( int int );
//!
