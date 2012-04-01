//!@expr{ GTK1.FontSelection()@}
//!@xml{<image>../images/gtk1_fontselection.png</image>@}
//!
//!
//!

inherit GTK1.Notebook;

protected GTK1.FontSelection create( );
//!

GDK1.Font get_font( );
//!

string get_font_name( );
//!

string get_preview_text( );
//!

GTK1.FontSelection set_font_name( string xlfd );
//! This sets the currently displayed font. It should be a valid X Logical
//! Font Description font name (anything else will be ignored), e.g.
//! "-adobe-courier-bold-o-normal--25-*-*-*-*-*-*-*"
//!
//!

GTK1.FontSelection set_preview_text( string text );
//!
