//! Menu items, to be added to menues.
//!
//!

inherit GTK.Item;

GTK.MenuItem activate( );
//! Emulate an activate signal
//!
//!

GTK.MenuItem configure( int toggle_indicator, int submenu_indicator );
//! First argument is 'show_toggle_indicator', second is
//! 'show_submenu_indicator'.
//!
//!

static GTK.MenuItem create( string|void label );
//! If a string is supplied, a W(Label) widget is created using that
//! string and added to the item. Otherwise, you should add another
//! widget to the list item with -&gt;add().
//!
//!

GTK.MenuItem deselect( );
//! Emulate a deselect signal
//!
//!

int get_accelerator_width( );
//! The width of the accelerator string, in pixels
//!
//!

int get_right_justify( );
//! Is the widget right justified?
//!
//!

int get_show_submenu_indicator( );
//! Should the submenu indicator be shown?
//!
//!

int get_show_toggle_indicator( );
//! Should the toggle indicator be shown?
//!
//!

int get_submenu_direction( );
//! The direction the submenu will be shown in. One of @[DIR_DOWN], @[DIR_LEFT], @[DIR_RIGHT], @[DIR_TAB_BACKWARD], @[DIR_TAB_FORWARD] and @[DIR_UP]
//!
//!

int get_submenu_placement( );
//! The placement of the submenu.
//!
//!

int get_toggle_size( );
//! The size of the toggle indicator
//!
//!

GTK.MenuItem remove_submenu( );
//! Remove the submenu for this menu button.
//!
//!

GTK.MenuItem right_justify( );
//! Make the menu item stick to the right edge of it's container.
//!
//!

GTK.MenuItem select( );
//! Emulate a select signal
//!
//!

GTK.MenuItem set_placement( int dir );
//! (sub menu placement) One of @[DIRECTION_LEFT] and @[DIRECTION_RIGHT]
//!
//!

GTK.MenuItem set_submenu( GTK.Widget menu );
//! Set the submenu for this menu button.
//!
//!
