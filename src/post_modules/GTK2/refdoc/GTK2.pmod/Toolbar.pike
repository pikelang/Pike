//! Toolbars are usually used to group some number of widgets in order
//! to simplify customization of their look and layout. Typically a
//! toolbar consists of buttons with icons, labels and tooltips, but
//! any other widget can also be put inside a toolbar. Finally, items
//! can be arranged horizontally or vertically and buttons can be
//! displayed with icons, labels, or both.
//! 
//! Examples:
//!@expr{ lambda(){object t=GTK2.Toolbar( /*GTK2.ORIENTATION_HORIZONTAL, GTK2.TOOLBAR_TEXT*/ );t->append_item( "Button 1", "Tooltip 1", "", GTK2.Frame(), lambda(){},0);t->append_space();t->append_item( "Button 2", "Tooltip 2", "", GTK2.Frame(), lambda(){},0);t->append_item( "Button 3", "Tooltip 3", "", GTK2.Frame(), lambda(){},0);t->append_space();t->append_item( "Button 4", "Tooltip 4", "", GTK2.Frame(), lambda(){},0);t->append_item( "Button 5", "Tooltip 5", "", GTK2.Frame(), lambda(){},0);return t;}()@}
//!@xml{<image>../images/gtk2_toolbar.png</image>@}
//!
//!@expr{ lambda(){object t=GTK2.Toolbar( /*GTK2.ORIENTATION_VERTICAL, GTK2.TOOLBAR_TEXT*/ );t->append_item( "Button 1", "Tooltip 1", "", GTK2.Frame(), lambda(){},0);t->append_space();t->append_item( "Button 2", "Tooltip 2", "", GTK2.Frame(), lambda(){},0);t->append_item( "Button 3", "Tooltip 3", "", GTK2.Frame(), lambda(){},0);t->append_space();t->append_item( "Button 4", "Tooltip 4", "", GTK2.Frame(), lambda(){},0);t->append_item( "Button 5", "Tooltip 5", "", GTK2.Frame(), lambda(){},0);return t;}()@}
//!@xml{<image>../images/gtk2_toolbar_2.png</image>@}
//!
//!@expr{ lambda(){object i=GTK2.GdkImage()->set(Image.Image(20,20)->test());object t=GTK2.Toolbar( /*GTK2.ORIENTATION_HORIZONTAL, GTK2.TOOLBAR_BOTH*/ );t->append_item( "Button 1", "Tooltip 1", "", GTK2.Image(i), lambda(){},0);t->append_space();t->append_item( "Button 2", "Tooltip 2", "", GTK2.Image(i), lambda(){},0);t->append_item( "Button 3", "Tooltip 3", "", GTK2.Image(i), lambda(){},0);t->append_space();t->append_item( "Button 4", "Tooltip 4", "", GTK2.Image(i), lambda(){},0);t->append_item( "Button 5", "Tooltip 5", "", GTK2.Image(i), lambda(){},0);return t;}()@}
//!@xml{<image>../images/gtk2_toolbar_3.png</image>@}
//!
//! 
//! Properties:
//! int orientation
//! int show-arrow
//! int toolbar-style
//! 
//! Child properties:
//! int expand
//! int homogeneous
//! 
//! Style properties:
//! int button-relief
//! int internal-padding
//! int shadow-type
//! int space-size
//! int space-style
//!
//!
//!  Signals:
//! @b{orientation_changed@}
//!
//! @b{popup_context_menu@}
//!
//! @b{style_changed@}
//!

inherit GTK2.Container;

static GTK2.Toolbar create( mapping|void props );
//! Creates a new toolbar.
//!
//!

int get_drop_index( int x, int y );
//! Returns the position corresponding to the indicated point on the toolbar.
//! This is useful when dragging items to the toolbar: this function returns
//! the position a new item should be inserted.
//! 
//! x and y are in toolbar coordinates.
//!
//!

int get_icon_size( );
//! Retrieves the icon size for the toolbar.  One of @[ICON_SIZE_BUTTON], @[ICON_SIZE_DIALOG], @[ICON_SIZE_DND], @[ICON_SIZE_INVALID], @[ICON_SIZE_LARGE_TOOLBAR], @[ICON_SIZE_MENU] and @[ICON_SIZE_SMALL_TOOLBAR].
//!
//!

int get_item_index( GTK2.ToolItem item );
//! Returns the position of item on the toolbar, starting from 0.
//!
//!

int get_n_items( );
//! Returns the number of items on the toolbar.
//!
//!

GTK2.ToolItem get_nth_item( int n );
//! Returns the n's item on the toolbar, or empty if the toolbar does not
//! contain an n'th item.
//!
//!

int get_orientation( );
//! Retrieves the current orientation of the toolbar.
//!
//!

int get_relief_style( );
//! Returns the relief style of buttons.
//!
//!

int get_show_arrow( );
//! Returns whether the toolbar has an overflow menu.
//!
//!

int get_style( );
//! Retrieves whether the toolbar has text, icons, or both.  One of
//! @[TOOLBAR_BOTH], @[TOOLBAR_BOTH_HORIZ], @[TOOLBAR_ICONS], @[TOOLBAR_SPACE_EMPTY], @[TOOLBAR_SPACE_LINE] and @[TOOLBAR_TEXT];
//!
//!

int get_tooltips( );
//! Retrieves whether tooltips are enabled.
//!
//!

GTK2.Toolbar insert( GTK2.ToolItem item, int pos );
//! Insert a W(ToolItem) into the toolbar at position pos.  If pos is 0
//! the item is prepended to the start of the toolbar.  If pos is negative,
//! the item is appended to the end of the toolbar.
//!
//!

GTK2.Toolbar set_drop_highlight_item( GTK2.ToolItem item, int index );
//! Highlights the toolbar to give an ide aof what it would like if item was
//! added at the position indicated by index.
//! 
//! The item passed to this function must not be part of any widget hierarchy.
//! When an item is set as drop highlight item it can not be added to any
//! widget hierarchy or used as highlight item for another toolbar.
//!
//!

GTK2.Toolbar set_orientation( int orientation );
//! Sets whether a toolbar should appear horizontally or vertically.
//! One of @[ORIENTATION_HORIZONTAL] and @[ORIENTATION_VERTICAL].
//!
//!

GTK2.Toolbar set_show_arrow( int show_arrow );
//! Sets whether to show an overflow menu when the toolbar doesn't have room
//! for all items on it.  If true, items for which there are not room are
//! are available through an overflow menu.
//!
//!

GTK2.Toolbar set_style( int style );
//! Set the style, one of @[TOOLBAR_BOTH], @[TOOLBAR_BOTH_HORIZ], @[TOOLBAR_ICONS], @[TOOLBAR_SPACE_EMPTY], @[TOOLBAR_SPACE_LINE] and @[TOOLBAR_TEXT]
//!
//!

GTK2.Toolbar set_tooltips( int enable );
//! Sets if the tooltips should be active or not.
//!
//!

GTK2.Toolbar unset_style( );
//! Unsets a toolbar style, so that user preferences will be used to
//! determine the toolbar style.
//!
//!
