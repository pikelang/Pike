// Automatically generated from "gtkcombo.pre".
// Do NOT edit.

//! Thee combo box is another fairly simple widget that is really just
//! a collection of other widgets. From the user's point of view, the
//! widget consists of a text entry box and a pull down menu from which
//! the user can select one of a set of predefined
//! entries. Alternatively, the user can type a different option
//! directly into the text box.
//! 
//! The combo box has two principal parts that you as the programmer
//! really care about: The W(entry) and the W(list).
//! 
//! DEPRECATED!!!
//! 
//!@expr{ GTK2.Combo()@}
//!@xml{<image>../images/gtk2_combo.png</image>@}
//!
//! Properties:
//! int allow-empty
//! int case-sensitive
//! int enable-arrow-keys
//! int enable-arrows-always
//! int value-in-list
//!
//!

inherit GTK2.Hbox;
//!

protected void create( );
//! Create a new combo box
//!
//!

GTK2.Combo disable_activate( );
//! This will disable the 'activate' signal for the entry widget in the
//! combo box.
//!
//!

GTK2.Entry get_entry( );
//! The entry widget
//!
//!

GTK2.Combo set_case_sensitive( int sensitivep );
//! set_case_sensitive() toggles whether or not GTK searches for
//! entries in a case sensitive manner. This is used when the Combo
//! widget is asked to find a value from the list using the current
//! entry in the text box. This completion can be performed in either a
//! case sensitive or insensitive manner, depending upon the use of
//! this function. The Combo widget can also simply complete the
//! current entry if the user presses the key combination MOD-1 and
//! "Tab". MOD-1 is often mapped to the "Alt" key, by the xmodmap
//! utility. Note, however that some window managers also use this key
//! combination, which will override its use within GTK.
//!
//!

GTK2.Combo set_item_string( GTK2.Item item, string text );
//! The item is one of the ones in the list subwidget.
//!
//!

GTK2.Combo set_popdown_strings( array strings );
//! Set the values in the popdown list.
//!
//!

GTK2.Combo set_use_arrows( int use_arrows );
//! set_use_arrows() lets the user change the value in the entry using
//! the up/down arrow keys. This doesn't bring up the list, but rather
//! replaces the current text in the entry with the next list entry (up
//! or down, as your key choice indicates). It does this by searching
//! in the list for the item corresponding to the current value in the
//! entry and selecting the previous/next item accordingly. Usually in
//! an entry the arrow keys are used to change focus (you can do that
//! anyway using TAB). Note that when the current item is the last of
//! the list and you press arrow-down it changes the focus (the same
//! applies with the first item and arrow-up).
//!
//!

GTK2.Combo set_use_arrows_always( int always_arrows );
//! set_use_arrows_always() allows the use the the up/down arrow keys
//! to cycle through the choices in the dropdown list, just as with
//! set_use_arrows, but it wraps around the values in the list,
//! completely disabling the use of the up and down arrow keys for
//! changing focus.
//!
//!

GTK2.Combo set_value_in_list( int value_must_be_in_list, int ok_if_empty );
//! If value_must_be_in_list is true, the user will not be able to
//! enter any value that is not in the list. If ok_if_empty is true,
//! empty values are possible as well as the values in the list.
//!
//!
