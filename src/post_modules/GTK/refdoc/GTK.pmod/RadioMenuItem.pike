//! Exactly like W(RadioButton), but it is an menu item.
//!@code{ GTK.RadioMenuItem("Menu item")@}
//!@xml{<image>../images/gtk_radiomenuitem.png</image>@}
//!
//!
//!

inherit GTK.MenuItem;

static GTK.RadioMenuItem create( string|void title, GTK.RadioMenuItem groupmember );
//! object GTK.RadioMenuItem(string title) - First button (with label)
//! object GTK.RadioMenuItem()->add(widget) - First button (with widget)
//! object GTK.RadioMenuItem(title, another_radio_button) - Second to n:th button (with title)
//! object GTK.RadioMenuItem(0,another_radio_button)->add(widget) - Second to n:th button (with widget)
//!
//!

GTK.RadioMenuItem set_group( GTK.RadioMenuItem groupmember );
//! the argument is another radio menu item to whose group this button
//! should be added to. It is prefereable to use the second argument to
//! the constructor instead, but if you for some reason want to move
//! the button to another group, use this function.
//!
//!
