//! Basically a horizontal W(Menu).
//! The menu image cannot be
//! grabbed automatically, but this is how you would create a menu all
//! in one line. This is not the recommended coding style.
//!@code{ GTK.MenuBar()->add(GTK.Menu_item("Menu")->set_submenu(GTK.Menu()->add(GTK.Menu_item("Sub")))->select()->activate())->add(GTK.Menu_item("Bar"))@}
//!@xml{<image src='../images/gtk_menubar.png'/>@}
//!
//! 
//!
//!

inherit MenuShell;

MenuBar append( GTK.Widget menu );
//!

static MenuBar create( );
//!

MenuBar insert( GTK.Widget menu, int position );
//!

MenuBar prepend( GTK.Widget menu );
//!

MenuBar set_shadow_type( int int );
//!
