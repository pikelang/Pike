//! Properties:
//! GTK2.ActionGroup action-group
//! int hide-if-empty
//! int is-important
//! string label
//! string name
//! int sensitive
//! string short-label
//! string stock-id
//! string tooltip
//! int visible
//! int visible-horizontal
//! int visible-overflown
//! int visible-vertical
//!
//!
//!  Signals:
//! @b{activate@}
//!

inherit G.Object;

GTK2.Action activate( );
//! Emits the "activate" signal, if it isn't insensitive.
//!
//!

GTK2.Action block_activate_from( GTK2.Widget proxy );
//! Disables calls to the activate() function by signals on the proxy. This is
//! used to break notification loops for things like check or radio actions.
//!
//!

GTK2.Action connect_accelerator( );
//! Installs the accelerator if this action widget has an accel path and group.
//!
//!

GTK2.Action connect_proxy( GTK2.Widget proxy );
//! Connects a widget to an action object as a proxy.  Synchronises various
//! properties of the action with the widget (such as label text, icon,
//! tooltip, etc), and attaches a callback so that the action gets activated
//! when the proxy widget does.
//!
//!

static GTK2.Action create( string|mapping name_or_props, string|void label, string|void tooltip, string|void stock_id );
//! Creates a new object.
//!
//!

GTK2.Widget create_icon( int icon_size );
//! This function is intended for use by action implementations to create
//! icons displayed in the proxy widgets.  One of @[ICON_SIZE_BUTTON], @[ICON_SIZE_DIALOG], @[ICON_SIZE_DND], @[ICON_SIZE_INVALID], @[ICON_SIZE_LARGE_TOOLBAR], @[ICON_SIZE_MENU] and @[ICON_SIZE_SMALL_TOOLBAR].
//!
//!

GTK2.Widget create_menu_item( );
//! Creates a menu item widget that proxies for the action.
//!
//!

GTK2.Widget create_tool_item( );
//! Creates a toolbar item widget that proxies for the action.
//!
//!

GTK2.Action disconnect_accelerator( );
//! Undoes the effect of one call to connect_accelerator().
//!
//!

GTK2.Action disconnect_proxy( GTK2.Widget proxy );
//! Disconnects a proxy widget.  Does not destroy the widget.
//!
//!

string get_accel_path( );
//! Returns the accel path for this action.
//!
//!

string get_name( );
//! Returns the name of the action.
//!
//!

array get_proxies( );
//! Returns the proxy widgets.
//!
//!

int get_sensitive( );
//! Returns whether the action itself is sensitive.  Note that this doesn't
//! necessarily mean effective sensitivity.
//!
//!

int get_visible( );
//! Returns whether the action itself is visible.
//!
//!

int is_sensitive( );
//! Returns whether the action is effectively sensitive.
//!
//!

int is_visible( );
//! Returns whether the action is effectively visible.
//!
//!

GTK2.Action set_accel_group( GTK2.AccelGroup group );
//! Sets the GTK2.AccelGroup in which the accelerator for this action will be
//! installed.
//!
//!

GTK2.Action set_accel_path( string accel_path );
//! Sets the accel path for this action.  All proxy widgets associated with
//! this action will have this accel path, so that their accelerators are
//! consistent.
//!
//!

GTK2.Action set_sensitive( int setting );
//! Sets the sensitive property.
//!
//!

GTK2.Action set_visible( int setting );
//! Sets the visible property.
//!
//!

GTK2.Action unblock_activate_from( GTK2.Widget proxy );
//! Re-enables calls to the activate() function.
//!
//!
