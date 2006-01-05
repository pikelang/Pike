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

int get_icon_size( );
//! Retrieves the icon size for the toolbar.  One of @[ICON_SIZE_BUTTON], @[ICON_SIZE_DIALOG], @[ICON_SIZE_DND], @[ICON_SIZE_INVALID], @[ICON_SIZE_LARGE_TOOLBAR], @[ICON_SIZE_MENU] and @[ICON_SIZE_SMALL_TOOLBAR].
//!
//!

int get_orientation( );
//! Retrieves the current orientation of the toolbar.
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

GTK2.Toolbar set_orientation( int orientation );
//! Sets whether a toolbar should appear horizontally or vertically.
//! One of @[ORIENTATION_HORIZONTAL] and @[ORIENTATION_VERTICAL].
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
