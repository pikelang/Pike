//! This widget is a wrapper around the GtkEntry widget, but it
//! provides a history mechanism for all the input entered into the
//! widget. The way this works is that a special identifier is provided
//! when creating the GnomeEntry widget, and this identifier is used to
//! load and save the history of the text.
//!
//!@code{ Gnome.Entry( "history" )@}
//!@xml{<image>../images/gnome_entry.png</image>@}
//!
//!
//!
//!

inherit GTK.Combo;

Gnome.Entry append_history( int save, string text );
//! Adds a history item of the given text to the tail of the history
//! list inside gentry. If save is TRUE, the history item will be saved
//! in the config file (assuming that gentry's history id is not 0).
//!
//!

static Gnome.Entry create( string|void history_id );
//! Creates a new GnomeEntry widget. If history_id is not 0, then
//! the history list will be saved and restored between uses under the
//! given id.
//!
//!

GTK.Entry gtk_entry( );
//! Obtain pointer to Gnome.Entry's internal GTK.Entry text entry
//!
//!

Gnome.Entry load_history( );
//! Loads a stored history list from the GNOME config file, if one is
//! available. If the history id of gentry is 0, nothing occurs.
//!
//!

Gnome.Entry prepend_history( int save, string text );
//! Adds a history item of the given text to the head of the history
//! list inside gentry. If save is TRUE, the history item will be saved
//! in the config file (assuming that gentry's history id is not 0).
//!
//!

Gnome.Entry save_history( );
//! Force the history items of the widget to be stored in a
//! configuration file. If the history id of gentry is 0, nothing
//! occurs.
//!
//!

Gnome.Entry set_history_id( string|void history_id );
//! Set or clear the history id of the GnomeEntry widget. If history_id
//! is 0, the widget's history id is cleared. Otherwise, the given
//! id replaces the previous widget history id.
//!
//!
