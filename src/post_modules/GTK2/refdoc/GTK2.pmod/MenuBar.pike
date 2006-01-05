//! Basically a horizontal W(Menu).
//! The menu image cannot be
//! grabbed automatically, but this is how you would create a menu all
//! in one line. This is not the recommended coding style.
//!@expr{ GTK2.MenuBar()->add(GTK2.MenuItem("Menu")->set_submenu(GTK2.Menu()->add(GTK2.MenuItem("Sub")))->select()->activate())->add(GTK2.MenuItem("Bar"))@}
//!@xml{<image>../images/gtk2_menubar.png</image>@}
//!
//! 
//! Properties:
//! int child-pack-direction
//! int pack-direction
//! 
//! Style properties:
//! int internal-padding
//! int shadow-type
//!
//!

inherit GTK2.MenuShell;

static GTK2.MenuBar create( mapping|void props );
//! Create a new menu bar.
//!
//!

int get_child_pack_direction( );
//! Retrieves the current child pack direction.
//!
//!

int get_pack_direction( );
//! Retrieves the current pack direction of the menubar.
//!
//!

GTK2.MenuBar set_child_pack_direction( int setting );
//! Sets how widgets should be packed inside the children of a menubar.
//!
//!

GTK2.MenuBar set_pack_direction( int setting );
//! Sets how items should be packed inside a menubar.  One of
//! .
//!
//!
