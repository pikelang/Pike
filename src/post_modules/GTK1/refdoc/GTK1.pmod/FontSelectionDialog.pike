//! The GtkFontSelectionDialog widget is a dialog box for selecting a font.
//! 
//! To set the font which is initially selected, use set_font_name().
//! 
//! To get the selected font use get_font() or get_font_name().
//! 
//! To change the text which is shown in the preview area, use
//! set_preview_text().
//! 
//! The base filter is not yet implemented in Pike GTK1.
//! @i{
//! Filters can be used to limit the fonts shown. There are 2 filters in
//! the GtkFontSelectionDialog - a base filter and a user filter. The
//! base filter can not be changed by the user, so this can be used when
//! the user must choose from the restricted set of fonts (e.g. for a
//! terminal-type application you may want to force the user to select a
//! fixed-width font). The user filter can be changed or reset by the
//! user, by using the 'Reset Filter' button or changing the options on
//! the 'Filter' page of the dialog.
//! @}
//!@expr{ GTK1.FontSelectionDialog("Select a font")@}
//!@xml{<image>../images/gtk1_fontselectiondialog.png</image>@}
//!
//!
//!

inherit GTK1.Window;

protected GTK1.FontSelectionDialog create( string title );
//! Create a new font selection dialog with the specified window title
//!
//!

GTK1.Button get_apply_button( );
//! The apply button
//!
//!

GTK1.Button get_cancel_button( );
//! The cancel button
//!
//!

GTK1.FontSelection get_fontsel( );
//! The W(FontSelection) widget
//!
//!

GTK1.Button get_ok_button( );
//! The ok button
//!
//!
