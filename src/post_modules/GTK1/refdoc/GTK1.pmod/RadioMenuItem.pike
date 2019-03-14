//! Exactly like W(RadioButton), but it is an menu item.
//!@expr{ GTK1.RadioMenuItem("Menu item")@}
//!@xml{<image>../images/gtk1_radiomenuitem.png</image>@}
//!
//!
//!

inherit GTK1.MenuItem;

protected GTK1.RadioMenuItem create( string|void title, GTK1.RadioMenuItem groupmember );
//! object GTK1.RadioMenuItem(string title) - First button (with label)
//! object GTK1.RadioMenuItem()->add(widget) - First button (with widget)
//! object GTK1.RadioMenuItem(title, another_radio_button) - Second to n:th button (with title)
//! object GTK1.RadioMenuItem(0,another_radio_button)->add(widget) - Second to n:th button (with widget)
//!
//!

GTK1.RadioMenuItem set_group( GTK1.RadioMenuItem groupmember );
//! the argument is another radio menu item to whose group this button
//! should be added to. It is prefereable to use the second argument to
//! the constructor instead, but if you for some reason want to move
//! the button to another group, use this function.
//!
//!
