//!

inherit GTK.Bin;

static Gnome.DockItem create( string name, int behavior );
//! Create a new GnomeDockItem named name, with the specified behavior.
//!  Gnome.DockItemBehExclusive specifies that the dock item is always the only one in its band. 
//!  Gnome.DockItemBehNeverFloating specifies that users cannot detach the dock item from the dock. 
//!  Gnome.DockItemBehNeverVertical specifies that the dock item must be kept horizontal, and users cannot move it to a vertical band.
//!  Gnome.DockItemBehNeverHorizontal specifies that the dock item must be kept horizontal, and users cannot move it to a vertical band.
//!  Gnome.DockItemBehLocked specifies that users cannot drag the item around.
//! 
//!
//!

int get_behavior( );
//!

GTK.Widget get_child( );
//! Retrieve the child of the item.
//!
//!

string get_name( );
//! Retrieve the name
//!
//!

int get_orientation( );
//!

int get_shadow_type( );
//! One of @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_OUT]
//!
//!

Gnome.DockItem set_orientation( int orientation );
//!

Gnome.DockItem set_shadow_type( int shadow_type );
//! One of @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_OUT]
//!
//!
