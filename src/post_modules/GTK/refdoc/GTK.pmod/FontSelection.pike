//!@code{ GTK.FontSelection()@}
//!@xml{<image>../images/gtk_fontselection.png</image>@}
//!
//!
//!

inherit GTK.Notebook;

static GTK.FontSelection create( );
//!

GDK.Font get_font( );
//!

string get_font_name( );
//!

string get_preview_text( );
//!

GTK.FontSelection set_font_name( string xlfd );
//! This sets the currently displayed font. It should be a valid X Logical
//! Font Description font name (anything else will be ignored), e.g.
//! "-adobe-courier-bold-o-normal--25-*-*-*-*-*-*-*"
//!
//!

GTK.FontSelection set_preview_text( string text );
//!
