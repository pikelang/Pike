//! The Gnome.DockLayout widget is meant to make it simple for
//! programmers to handle the layout of a GnomeDock widget.
//! 
//!  Gnome.DockLayout can contain an arbitrary number of W(Gnome.DockItem)
//!  widgets, each of them with its own placement information. It is
//!  possible to "extract" a layout from an existing W(GnomeDock)
//!  widget, as well as adding the items present in a Gnome.DockLayout
//!  to it. Moreover, Gnome.DockLayout is able to create a layout
//!  configuration string that can be later used to re-construct the
//!  layout on a brand new Gnome.DockLayout widget.
//! 
//! As a consequence, Gnome.DockLayout is very useful to save and
//! retrieve W(GnomeDock) configurations into files. For example,
//! W(GnomeApp) uses Gnome.DockLayout to create a default layout
//! configuration, override it with the user-specific configuration
//! file, and finally apply it to it's W(GnomeDock).
//! 
//!
//!

inherit Object;

int add_floating_item( Gnome.DockItem item, int x, int y, int orientation );
//! Add item to the layout as a floating item with the specified (x, y)
//! position and orientation.
//!
//!

int add_item( Gnome.DockItem item, int placement, int band_num, int band_position, int offset );
//! Add item to the layout with the specified parameters.
//!
//!

int add_to_dock( Gnome.Dock dock );
//! Add all the items in this layout to the specified dock
//!
//!

static GnomeDockLayout create( );
//! Create a new Gnome.DockLayout widget.
//!
//!

string create_string( );
//! Generate a string describing the layout
//!
//!

Gnome.DockLayoutItem get_item( Gnome.DockItem item );
//!  Retrieve a dock layout item.
//!
//!

Gnome.DockLayoutItem get_item_by_name( string name );
//! Retrieve the dock layout item named name
//!
//!

GnomeDockLayout parse_string( string str );
//! Parse the layout string str, and move around the items in layout
//! accordingly.
//!
//!

int remove_item( Gnome.DockItem item );
//!  Remove the specified item from the layout.
//!
//!

int remove_item_by_name( string name );
//!  Remove the specified item from the layout.
//!
//!
