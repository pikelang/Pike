//! This is a W(GnomeDruidPage). It is meant to be used to introduce
//! the user to what is being installed in a consistent manner.
//!
//!

inherit Gnome.DruidPage;

static Gnome.DruidPageStart create( );
//! Creates a new Gnome.DruidPageStart widget.
//!
//!

Gnome.DruidPageStart set_bg_color( GDK.Color color );
//! This will set the background color to be the specified color.
//!
//!

Gnome.DruidPageStart set_logo_bg_color( GDK.Color color );
//! This will set the background color of the logo
//!
//!

Gnome.DruidPageStart set_text( string text );
//! Set the text
//!
//!

Gnome.DruidPageStart set_text_color( GDK.Color color );
//! Set the text color
//!
//!

Gnome.DruidPageStart set_textbox_color( GDK.Color color );
//! This will set the textbox color to be the specified color.
//!
//!

Gnome.DruidPageStart set_title( string title );
//! Set the title
//!
//!

Gnome.DruidPageStart set_title_color( GDK.Color color );
//! Set the title color
//!
//!
