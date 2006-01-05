//! The W(FontSelection) widget lists the available fonts, styles and sizes,
//! allowing the user to select a font.  It is used in W(FontSelectionDialog).
//!@expr{ GTK2.FontSelection();@}
//!@xml{<image>../images/gtk2_fontselection.png</image>@}
//!
//! Properties:
//! string font-name
//! string preview-text
//!
//!

inherit GTK2.Vbox;

static GTK2.FontSelection create( mapping|void props );
//! Create a new GTK2.FontSelection widget.
//!
//!

string get_font_name( );
//! Gets the currently-selected font name.
//!
//!

string get_preview_text( );
//! Gets the text displayed in the preview area.
//!
//!

GTK2.FontSelection set_font_name( string fontname );
//! This sets the currently displayed font. 
//!
//!

GTK2.FontSelection set_preview_text( string text );
//! Sets the text displayed in the preview area.
//!
//!
