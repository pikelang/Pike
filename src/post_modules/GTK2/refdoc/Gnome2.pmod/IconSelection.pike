// Automatically generated from "gnomeiconselection.pre".
// Do NOT edit.

//! An icon listing/chooser display.
//!
//!

inherit GTK2.Vbox;
//!

Gnome2.IconSelection add_defaults( );
//! Adds the default pixmap directory into the selection widget.
//!
//!

Gnome2.IconSelection add_directory( string dir );
//! Adds the icons from the directory dir to the selection widget.
//!
//!

Gnome2.IconSelection clear( int|void not_shown );
//! Clear the currently shown icons, the ones that weren't shown yet
//! are not cleared unless the not_shown parameter is given, in which
//! case even those are cleared.
//!
//!

protected void create( );
//! Creates a new icon selection widget, it uses a W(GnomeIconList) for
//! the listing of icons
//!
//!

GTK2.Widget get_box( );
//! Gets the W(Vbox) widget.
//!
//!

string get_icon( int full_path );
//! Gets the currently selected icon name, if full_path is true, it
//! returns the full path to the icon, if none is selected it returns
//! 0.
//!
//!

Gnome2.IconSelection select_icon( string filename );
//! Selects the icon filename. This icon must have already been added
//! and shown
//!
//!

Gnome2.IconSelection show_icons( );
//! Shows the icons inside the widget that were added with add_defaults
//! and add_directory. Before this function isf called the icons aren't
//! actually added to the listing and can't be picked by the user.
//!
//!

Gnome2.IconSelection stop_loading( );
//! Stop the loading of images when we are in the loop in show_icons.
//!
//!
