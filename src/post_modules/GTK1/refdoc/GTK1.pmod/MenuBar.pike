//! Basically a horizontal W(Menu).
//! The menu image cannot be
//! grabbed automatically, but this is how you would create a menu all
//! in one line. This is not the recommended coding style.
//!@expr{ GTK1.MenuBar()->add(GTK1.MenuItem("Menu")->set_submenu(GTK1.Menu()->add(GTK1.MenuItem("Sub")))->select()->activate())->add(GTK1.MenuItem("Bar"))@}
//!@xml{<image>../images/gtk1_menubar.png</image>@}
//!
//! 
//!
//!

inherit GTK1.MenuShell;

GTK1.MenuBar append( GTK1.Widget menu );
//!

protected GTK1.MenuBar create( );
//!

GTK1.MenuBar insert( GTK1.Widget menu, int position );
//!

GTK1.MenuBar prepend( GTK1.Widget menu );
//!

GTK1.MenuBar set_shadow_type( int int );
//!
