//! Exactly like W(RadioButton), but it is an menu item.
//!@expr{ GTK2.RadioMenuItem("Menu item")@}
//!@xml{<image>../images/gtk2_radiomenuitem.png</image>@}
//!
//!
//!
//!  Signals:
//! @b{group_changed@}
//!

inherit GTK2.CheckMenuItem;

static GTK2.RadioMenuItem create( string|mapping title, GTK2.RadioMenuItem groupmember );
//! object GTK2.RadioMenuItem(string title) - First button (with label)
//! object GTK2.RadioMenuItem()->add(widget) - First button (with widget)
//! object GTK2.RadioMenuItem(title, another_radio_button) - Second to n:th button (with title)
//! object GTK2.RadioMenuItem(0,another_radio_button)->add(widget) - Second to n:th button (with widget)
//!
//!

array get_group( );
//! Returns the group to which the radio menu item belongs.
//!
//!

GTK2.RadioMenuItem set_group( GTK2.RadioMenuItem groupmember );
//! The argument is another radio menu item to whose group this button
//! should be added to. It is prefereable to use the second argument to
//! the constructor instead, but if you for some reason want to move
//! the button to another group, use this function.
//!
//!
