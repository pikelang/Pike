//! The GtkFontSelectionDialog widget is a dialog box for selecting a font.
//! 
//! To set the font which is initially selected, use set_font_name().
//! 
//! To get the selected font use get_font_name().
//! 
//! To change the text which is shown in the preview area, use
//! set_preview_text().
//! 
//!@expr{ GTK2.FontSelectionDialog("Select a font")@}
//!@xml{<image>../images/gtk2_fontselectiondialog.png</image>@}
//!
//!
//!

inherit GTK2.Dialog;

static GTK2.FontSelectionDialog create( string title );
//! Create a new font selection dialog with the specified window title
//!
//!

GTK2.Button get_apply_button( );
//! The apply button
//!
//!

GTK2.Button get_cancel_button( );
//! The cancel button
//!
//!

string get_font_name( );
//! Gets the currently-selected font name.
//!
//!

GTK2.Button get_ok_button( );
//! The ok button
//!
//!

string get_preview_text( );
//! Gets the text displayed in the preview area.
//!
//!

int set_font_name( string font );
//! Sets the currently-selected font.
//!
//!

GTK2.FontSelectionDialog set_preview_text( string text );
//! Sets the text displayed in the preview area.
//!
//!
