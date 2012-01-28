//! A OptionMenu is a widget that allows the user to choose from a
//! list of valid choices. The OptionMenu displays the selected
//! choice. When activated the OptionMenu displays a popup W(Menu)
//! which allows the user to make a new choice.
//! 
//!@expr{ GTK1.OptionMenu()->set_menu(GTK1.Menu()->add( GTK1.MenuItem("Option 1") ));@}
//!@xml{<image>../images/gtk1_optionmenu.png</image>@}
//!
//! 
//!
//!

inherit GTK1.Button;

protected GTK1.OptionMenu create( );
//! Create a new option menu widget
//!
//!

GTK1.Menu get_menu( );
//! Returns the W(Menu) associated with the OptionMenu.
//!
//!

GTK1.OptionMenu remove_menu( );
//! Remove the menu.
//!
//!

GTK1.OptionMenu set_history( int index );
//! Selects the menu item specified by index making it the newly
//! selected value for the option menu.
//!
//!

GTK1.OptionMenu set_menu( GTK1.Menu menu );
//! Provides the GtkMenu that is popped up to allow the user to choose
//! a new value. You should provide a simple menu avoiding the use of
//! tearoff menu items, submenus, and accelerators.
//!
//!
