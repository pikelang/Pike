//! Menu items, to be added to menus.
//! Style properties:
//! int arrow-spacing
//! int horizontal-padding
//! int selected-shadow-type
//! int toggle-spacing
//!
//!
//!  Signals:
//! @b{activate@}
//!
//! @b{activate_item@}
//!
//! @b{toggle_size_allocate@}
//!
//! @b{toggle_size_request@}
//!

inherit GTK2.Item;

GTK2.MenuItem activate( );
//! Emulate an activate signal
//!
//!

static GTK2.MenuItem create( string|mapping label_or_props );
//! If a string is supplied, a W(Label) widget is created using that
//! string and added to the item. Otherwise, you should add another
//! widget to the list item with -&gt;add().
//!
//!

GTK2.MenuItem deselect( );
//! Emulate a deselect signal
//!
//!

int get_right_justified( );
//! Gets whether the menu item appears justified at the right side of the menu
//! bar.
//!
//!

GTK2.Widget get_submenu( );
//! Gets the submenu underneath this menu item.
//!
//!

GTK2.MenuItem remove_submenu( );
//! Remove the submenu for this menu button.
//!
//!

GTK2.MenuItem select( );
//! Emulate a select signal
//!
//!

GTK2.MenuItem set_accel_path( string path );
//! Sets the accelerator path.
//!
//!

GTK2.MenuItem set_right_justified( int setting );
//! Make the menu item stick to the right edge of it's container.
//!
//!

GTK2.MenuItem set_submenu( GTK2.Widget menu );
//! Set the submenu for this menu button.
//!
//!

GTK2.MenuItem toggle_size_allocate( int allocation );
//! Emits the "toggle-size-allocate" signal on the given item.
//!
//!

int toggle_size_request( int requisition );
//! Emits the "toggle-size-request" signal on the given item.
//!
//!
