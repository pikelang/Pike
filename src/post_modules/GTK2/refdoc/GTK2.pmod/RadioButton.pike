//! Radio buttons are similar to check buttons except they are grouped
//! so that only one may be selected/depressed at a time. This is good
//! for places in your application where you need to select from a
//! short list of options.
//!  To connect the buttons, use another button in the desired group
//! as the second argument to GTK2.RadioButton().
//! 
//!@expr{ GTK2.RadioButton("Button");@}
//!@xml{<image>../images/gtk2_radiobutton.png</image>@}
//!
//! Properties:
//! GTK2.RadioButton group
//!
//!
//!  Signals:
//! @b{group_changed@}
//!

inherit GTK2.CheckButton;

static GTK2.RadioButton create( string|mapping title, GTK2.RadioButton groupmember );
//!  Normal creation:
//! object GTK2.RadioButton(string title) - First button (with label)
//! object GTK2.RadioButton()->add(widget) - First button (with widget)
//! object GTK2.RadioButton(title, another_radio_button) - Second to n:th button (with title)
//! object GTK2.RadioButton(0,another_radio_button)->add(widget) - Second to n:th button (with widget)
//! 
//!
//!

array get_group( );
//! Returns an array of members in this group.
//!
//!

GTK2.RadioButton set_group( GTK2.RadioButton groupmember );
//! the argument is another radio button to whose group this button
//! should be added to. It is prefereable to use the second argument to
//! the constructor instead, but if you for some reason want to move
//! the button to another group, use this function.
//!
//!
