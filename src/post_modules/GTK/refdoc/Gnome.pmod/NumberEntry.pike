//!  Provides an entry line for numbers. This routine does not attempt
//!  to do any validation on the valid number ranges, but provides a
//!  button that will let the user bring up a calculator to fill in the
//!  value of the entry widget.
//!@expr{ Gnome.NumberEntry("", "Select a number...");@}
//!@xml{<image>../images/gnome_numberentry.png</image>@}
//!
//!
//!

inherit GTK.Hbox;

static Gnome.NumberEntry create( string history_id, string calc_dialog_title );
//! Creates a new number entry widget, with a history id and title for
//! the calculator dialog.
//!
//!

float get_number( );
//! Get the current number from the entry
//!
//!

Gnome.Entry gnome_entry( );
//! Get the W(GnomeEntry) component of the Gnome.NumberEntry for
//! lower-level manipulation.
//!
//!

GTK.Entry gtk_entry( );
//! Get the W(Entry) component of the Gnome.NumberEntry for Gtk+-level
//! manipulation.
//!
//!

Gnome.NumberEntry set_title( string title );
//! Set the title of the calculator dialog to calc_dialog_title. Takes
//! effect the next time the calculator button is pressed.
//!
//!
