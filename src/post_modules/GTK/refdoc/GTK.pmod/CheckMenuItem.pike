//! A check menu item is more or less identical to a check button, but it
//! should be used in menus.
//!@code{ GTK.CheckMenuItem("Hi there")@}
//!@xml{<image src='../images/gtk_checkmenuitem.png'/>@}
//!
//!@code{ GTK.CheckMenuItem("Hi there")->set_active(1)@}
//!@xml{<image src='../images/gtk_checkmenuitem_2.png'/>@}
//!
//!
//!
//!  Signals:
//! @b{toggled@}
//! Called when the state of the menu item is changed
//!
//!

inherit GTK.MenuItem;

static GTK.CheckMenuItem create( string|void label );
//! The argument, if specified, is the label of the item.
//! If no label is specified, use object->add() to add some
//! other widget (such as an pixmap or image widget)
//!
//!

GTK.CheckMenuItem set_active( int new_state );
//! State is either 1 or 0. If 1, the button will be 'pressed'.
//!
//!

GTK.CheckMenuItem set_show_toggle( int togglep );
//! If true, the toggle indicator will be shown
//!
//!

GTK.CheckMenuItem toggled( );
//! Emulate a toggled event
//!
//!
