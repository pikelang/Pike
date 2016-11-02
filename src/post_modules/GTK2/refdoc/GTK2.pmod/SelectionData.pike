//! Drag-and-drop selection data transport object.
//! Most commonly accessed from selection handler callbacks.
//!
//!

string get_text( );
//! Retrieve the selection data as a string.
//!
//!

GTK2.SelectionData set_text( string text );
//! Set the selection data to the given text string.
//!
//!
