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

static GTK2.ColorButton create( GTK2.GdkColor red_or_props, int|void green, int|void blue );
//! Create a new W(ColorButton).
//!
//!

int get_alpha( );
//! Get the current alpha value.
//!
//!

GTK2.GdkColor get_color( );
//! Returns the current color.
//!
//!

mixed get_property( string property );
//! Get property.
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

GTK2.ColorButton set_color( int red, int green, int blue );
//! Sets the current color.
//!
//!

GTK2.ColorButton set_property( string property, mixed value );
//! Set property.
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
