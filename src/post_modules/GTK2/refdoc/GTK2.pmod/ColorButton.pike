// Automatically generated from "gtkcolorbutton.pre".
// Do NOT edit.

//! Properties:
//! int alpha
//! GDK2.Color color
//! string title
//! int use-alpha
//!
//!
//!  Signals:
//! @b{color_set@}
//! When a user selects a color.
//!
//!

inherit GTK2.Button;
//!

protected void create( void red_or_props, void green, void blue );
//! Create a new W(ColorButton).
//!
//!

int get_alpha( );
//! Get the current alpha value.
//!
//!

GDK2.Color get_color( );
//! Returns the current color.
//!
//!

string get_title( );
//! Get the title.
//!
//!

int get_use_alpha( );
//! Gets whether the color button uses the alpha channel.
//!
//!

GTK2.ColorButton set_alpha( int alpha );
//! Sets the current opacity to alpha.
//!
//!

GTK2.ColorButton set_color( int|GdkColor red, int|void green, int|void blue );
//! Sets the current color.
//!
//!

GTK2.ColorButton set_title( string title );
//! Sets the title for the color selection dialog.
//!
//!

GTK2.ColorButton set_use_alpha( int use_alpha );
//! Sets whether or not the color button should use the alpha channel.
//!
//!
