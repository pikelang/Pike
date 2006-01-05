//! Properties:
//! string font-name
//! int show-size
//! int show-style
//! string title
//! int use-font
//! int use-size
//!
//!
//!  Signals:
//! @b{font_set@}
//!

inherit GTK2.Button;

static GTK2.FontButton create( string|mapping font_or_props );
//! Create a new font picker widget.  If font is specified, it will be
//! displayed in the font selection dialog.
//!
//!

string get_font_name( );
//! Retrieves the name of the currently selected font.
//!
//!

int get_show_size( );
//! Returns whether the font size will be shown in the label.
//!
//!

int get_show_style( );
//! Returns whether the name of the font style will be shown in the label.
//!
//!

string get_title( );
//! Retrieves the title of the font selection dialog.
//!
//!

int get_use_font( );
//! Returns whether the selected font is used in the label.
//!
//!

int get_use_size( );
//! Returns whether the selected size is used in the label.
//!
//!

int set_font_name( string font );
//! Sets or updates the currently-displayed font.
//!
//!

GTK2.FontButton set_show_size( int setting );
//! If setting is true, the font size will be displayed along with the name of
//! the selected of the selected font.
//!
//!

GTK2.FontButton set_show_style( int setting );
//! If setting is true, the font style will be displayed along with the name
//! of the selected font.
//!
//!

GTK2.FontButton set_title( string title );
//! Sets the title for the font selection dialog.
//!
//!

GTK2.FontButton set_use_font( int setting );
//! If setting is true, the font name will be written using the selected font.
//!
//!

GTK2.FontButton set_use_size( int setting );
//! If setting is true, the font name will be written using the selected size.
//!
//!
