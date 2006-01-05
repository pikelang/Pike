//! A check menu item is more or less identical to a check button, but it
//! should be used in menus.
//!@expr{ GTK2.CheckMenuItem("Hi there")@}
//!@xml{<image>../images/gtk2_checkmenuitem.png</image>@}
//!
//!@expr{ GTK2.CheckMenuItem("Hi there")->set_active(1)@}
//!@xml{<image>../images/gtk2_checkmenuitem_2.png</image>@}
//!
//! Properties:
//! int active
//! int draw-as-radio
//! int inconsistent
//! 
//! Style properties:
//! int indicator-size
//!
//!
//!  Signals:
//! @b{toggled@}
//! Called when the state of the menu item is changed
//!
//!

inherit GTK2.MenuItem;

static GTK2.CheckMenuItem create( string|mapping label_or_props );
//! The argument, if specified, is the label of the item.
//! If no label is specified, use object->add() to add some
//! other widget (such as an pixmap or image widget)
//!
//!

int get_active( );
//! Get whether item is active.
//!
//!

int get_inconsistent( );
//! Retrieves the value set by set_inconsistent().
//!
//!

GTK2.CheckMenuItem set_active( int new_state );
//! State is either 1 or 0. If 1, the button will be 'pressed'.
//!
//!

GTK2.CheckMenuItem set_inconsistent( int setting );
//! If the user has selected a range of elements (such as some text or
//! spreadsheet cells) that are affected by a boolean setting, and the current
//! values in that range are inconsistent, you may want to display the check
//! in an "in between" state.  This function turns on "in between" display.
//!
//!

GTK2.CheckMenuItem toggled( );
//! Emulate a toggled event
//!
//!
