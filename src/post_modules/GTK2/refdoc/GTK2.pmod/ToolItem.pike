//! Properties:
//! int is-important
//! int visible-horizontal
//! int visible-vertical
//!
//!
//!  Signals:
//! @b{create_menu_proxy@}
//!
//! @b{set_tooltip@}
//!
//! @b{toolbar_reconfigured@}
//!

inherit GTK2.Bin;

static GTK2.ToolItem create( mapping|void props );
//! Creates a new GTK2.ToolItem.
//!
//!

int get_expand( );
//! Returns whether this item is allocated extra space.
//!
//!

int get_homogeneous( );
//! Returns whether this item is the same size as the other
//! homogeneous items.
//!
//!

int get_icon_size( );
//! Returns the icon size used for this item.
//! One of @[ICON_SIZE_BUTTON], @[ICON_SIZE_DIALOG], @[ICON_SIZE_DND], @[ICON_SIZE_INVALID], @[ICON_SIZE_LARGE_TOOLBAR], @[ICON_SIZE_MENU] and @[ICON_SIZE_SMALL_TOOLBAR]
//!
//!

int get_is_important( );
//! Returns whether this item is considered important.
//!
//!

int get_orientation( );
//! Returns the orientation used for this item.
//! One of @[ORIENTATION_HORIZONTAL] and @[ORIENTATION_VERTICAL]
//!
//!

GTK2.Widget get_proxy_menu_item( string menu_item_id );
//! If menu_item_id matches the string passed to
//! set_proxy_menu_item(), then return the corresponding
//! GTK2.MenuItem.
//!
//!

int get_relief_style( );
//! Returns the relief style of this item.
//! One of @[RELIEF_HALF], @[RELIEF_NONE] and @[RELIEF_NORMAL]
//!
//!

int get_toolbar_style( );
//! Returns the toolbar style use for this item.
//! One of @[TOOLBAR_BOTH], @[TOOLBAR_BOTH_HORIZ], @[TOOLBAR_ICONS], @[TOOLBAR_SPACE_EMPTY], @[TOOLBAR_SPACE_LINE] and @[TOOLBAR_TEXT]
//!
//!

int get_use_drag_window( );
//! Returns whether this item has a drag window.
//!
//!

int get_visible_horizontal( );
//! Returns whether this item is visible on toolbars that
//! are docked horizontally.
//!
//!

int get_visible_vertical( );
//! Returns whether this item is visible when the toolbar is
//! docked vertically.
//!
//!

GTK2.ToolItem rebuild_menu( );
//! Calling this function signals to the toolbar that the overflow
//! menu item for this item has changed.  If the overflow menu is
//! visible when this function is called, the menu will be rebuilt.
//!
//!

GTK2.Widget retrieve_proxy_menu_item( );
//! Returns the GTK2.MenuItem that was last set by
//! set_proxy_menu_item().
//!
//!

GTK2.ToolItem set_expand( int expand );
//! Sets whether this item is allocated extra space when there
//! is more room on the toolbar than needed for the items.  The
//! effect is that the item gets bigger when the toolbar gets
//! bigger and smaller when the toolbar gets smaller.
//!
//!

GTK2.ToolItem set_homogeneous( int homogeneous );
//! Sets whether this item is to be allocated the same size
//! as other homogeneous items.  The effect is that all
//! homogeneous items will have the same width as the widest
//! of the items.
//!
//!

GTK2.ToolItem set_is_important( int is_important );
//! Sets whether this item should be considered important.
//!
//!

GTK2.ToolItem set_proxy_menu_item( string menu_item_id, GTK2.Widget menu_item );
//! Sets the GTK2.MenuItem used in the toolbar overflow menu.
//! The menu_item_id is used to identify the caller of this
//! function and should also be used with get_proxy_menu_item().
//!
//!

GTK2.ToolItem set_tooltip( GTK2.Tooltips tooltips, string tip_text, string tip_private );
//! Sets the GTK2.Tooltips object to be used for this item,
//! the text to be displayed as tool tip on the item and
//! the private text to be used.
//!
//!

GTK2.ToolItem set_use_drag_window( int use_drag_window );
//! Sets whether this item has a drag window.  When true
//! the toolitem can be used as a drag source.  When this item
//! has a drag window it will intercept all events, even those
//! that would otherwise be sent to a child.
//!
//!

GTK2.ToolItem set_visible_horizontal( int visible_horizontal );
//! Sets whether toolitem is visible when the toolbar
//! is docked horizontally.
//!
//!

GTK2.ToolItem set_visible_vertical( int visible_vertical );
//! Sets whether this item is visible when the toolbar is
//! docked vertically.  Some tool items, such as text entries,
//! are too wide to be useful on a vertically docked toolbar.
//! If visible_vertical is false then this item will not appear
//! on toolbars that are docked vertically.
//!
//!
