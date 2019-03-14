//! A check menu item is more or less identical to a check button, but it
//! should be used in menus.
//!@expr{ GTK1.CheckMenuItem("Hi there")@}
//!@xml{<image>../images/gtk1_checkmenuitem.png</image>@}
//!
//!@expr{ GTK1.CheckMenuItem("Hi there")->set_active(1)@}
//!@xml{<image>../images/gtk1_checkmenuitem_2.png</image>@}
//!
//!
//!
//!  Signals:
//! @b{toggled@}
//! Called when the state of the menu item is changed
//!
//!

inherit GTK1.MenuItem;

protected GTK1.CheckMenuItem create( string|void label );
//! The argument, if specified, is the label of the item.
//! If no label is specified, use object->add() to add some
//! other widget (such as an pixmap or image widget)
//!
//!

GTK1.CheckMenuItem set_active( int new_state );
//! State is either 1 or 0. If 1, the button will be 'pressed'.
//!
//!

GTK1.CheckMenuItem set_show_toggle( int togglep );
//! If true, the toggle indicator will be shown
//!
//!

GTK1.CheckMenuItem toggled( );
//! Emulate a toggled event
//!
//!
