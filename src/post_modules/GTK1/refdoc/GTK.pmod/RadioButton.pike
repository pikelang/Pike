//! Radio buttons are similar to check buttons except they are grouped
//! so that only one may be selected/depressed at a time. This is good
//! for places in your application where you need to select from a
//! short list of options.
//!  To connect the buttons, use another button in the desired group
//! as the second argument to GTK.RadioButton().
//! 
//!@expr{ GTK.Radio_button("Button");@}
//!@xml{<image>../images/gtk_radiobutton.png</image>@}
//!
//!
//!

inherit GTK.CheckButton;

static GTK.RadioButton create( string|void title, GTK.RadioButton groupmember );
//!  Normal creation:
//! object GTK.RadioButton(string title) - First button (with label)
//! object GTK.RadioButton()->add(widget) - First button (with widget)
//! object GTK.RadioButton(title, another_radio_button) - Second to n:th button (with title)
//! object GTK.RadioButton(0,another_radio_button)->add(widget) - Second to n:th button (with widget)
//! 
//!
//!

GTK.RadioButton set_group( GTK.RadioButton groupmember );
//! the argument is another radio button to whose group this button
//! should be added to. It is prefereable to use the second argument to
//! the constructor instead, but if you for some reason want to move
//! the button to another group, use this function.
//!
//!
