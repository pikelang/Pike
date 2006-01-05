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
