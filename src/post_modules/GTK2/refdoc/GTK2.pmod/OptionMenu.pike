//! A OptionMenu is a widget that allows the user to choose from a
//! list of valid choices. The OptionMenu displays the selected
//! choice. When activated the OptionMenu displays a popup W(Menu)
//! which allows the user to make a new choice.
//! 
//!@expr{ GTK2.OptionMenu()->set_menu(GTK2.Menu()->add( GTK2.MenuItem("Option 1") ));@}
//!@xml{<image>../images/gtk2_optionmenu.png</image>@}
//!
//! 
//! Properties:
//! GTK2.Menu menu
//! Style properties:
//! int indicator-size
//! int indicator-spacing
//!
//!
//!  Signals:
//! @b{changed@}
//!

inherit GTK2.Button;

static GTK2.OptionMenu create( );
//! Create a new option menu widget
//!
//!

int get_history( );
//! Retrieves the index of the currently selected menu item.  The menu items
//! are numbered from top to bottom, starting with 0.
//!
//!

GTK2.Menu get_menu( );
//! Returns the W(Menu) associated with the OptionMenu.
//!
//!

GTK2.OptionMenu remove_menu( );
//! Remove the menu.
//!
//!

GTK2.OptionMenu set_history( int index );
//! Selects the menu item specified by index making it the newly
//! selected value for the option menu.
//!
//!

GTK2.OptionMenu set_menu( GTK2.Menu menu );
//! Provides the GtkMenu that is popped up to allow the user to choose
//! a new value. You should provide a simple menu avoiding the use of
//! tearoff menu items, submenus, and accelerators.
//!
//!
