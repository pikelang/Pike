//! Toolbars are usually used to group some number of widgets in order
//! to simplify customization of their look and layout. Typically a
//! toolbar consists of buttons with icons, labels and tooltips, but
//! any other widget can also be put inside a toolbar. Finally, items
//! can be arranged horizontally or vertically and buttons can be
//! displayed with icons, labels, or both.
//! 
//! Examples:
//!@code{ lambda(){object t=GTK.Toolbar( GTK.ORIENTATION_HORIZONTAL, GTK.TOOLBAR_TEXT );t->append_item( "Button 1", "Tooltip 1", "", GTK.Frame(), lambda(){},0);t->append_space();t->append_item( "Button 2", "Tooltip 2", "", GTK.Frame(), lambda(){},0);t->append_item( "Button 3", "Tooltip 3", "", GTK.Frame(), lambda(){},0);t->append_space();t->append_item( "Button 4", "Tooltip 4", "", GTK.Frame(), lambda(){},0);t->append_item( "Button 5", "Tooltip 5", "", GTK.Frame(), lambda(){},0);return t;}()@}
//!@xml{<image>../images/gtk_toolbar.png</image>@}
//!
//!@code{ lambda(){object t=GTK.Toolbar( GTK.ORIENTATION_VERTICAL, GTK.TOOLBAR_TEXT );t->append_item( "Button 1", "Tooltip 1", "", GTK.Frame(), lambda(){},0);t->append_space();t->append_item( "Button 2", "Tooltip 2", "", GTK.Frame(), lambda(){},0);t->append_item( "Button 3", "Tooltip 3", "", GTK.Frame(), lambda(){},0);t->append_space();t->append_item( "Button 4", "Tooltip 4", "", GTK.Frame(), lambda(){},0);t->append_item( "Button 5", "Tooltip 5", "", GTK.Frame(), lambda(){},0);return t;}()@}
//!@xml{<image>../images/gtk_toolbar_2.png</image>@}
//!
//!@code{ lambda(){object i=GDK.Image()->set(Image.image(20,20)->test());object t=GTK.Toolbar( GTK.ORIENTATION_HORIZONTAL, GTK.TOOLBAR_BOTH );t->append_item( "Button 1", "Tooltip 1", "", GTK.Image(i), lambda(){},0);t->append_space();t->append_item( "Button 2", "Tooltip 2", "", GTK.Image(i), lambda(){},0);t->append_item( "Button 3", "Tooltip 3", "", GTK.Image(i), lambda(){},0);t->append_space();t->append_item( "Button 4", "Tooltip 4", "", GTK.Image(i), lambda(){},0);t->append_item( "Button 5", "Tooltip 5", "", GTK.Image(i), lambda(){},0);return t;}()@}
//!@xml{<image>../images/gtk_toolbar_3.png</image>@}
//!
//! 
//!
//!
//!  Signals:
//! @b{orientation_changed@}
//!
//! @b{style_changed@}
//!

inherit GTK.Container;

GTK.Toolbar append_item( string label, string tooltip, string prv, GTK.Widget icon, function clicked_cb, mixed clicked_arg );
//! Adds a new button to the start of the toolbar.
//!
//!

GTK.Toolbar append_space( );
//! Adds a small space.
//!
//!

GTK.Toolbar append_widget( GTK.Widget widget, string tootip, string prv );
//! Append a custom widgets. Arguments are widget, tooltip, private
//!
//!

static GTK.Toolbar create( int orientation, int style );
//! Orientation is one of
//! @[ORIENTATION_HORIZONTAL] and @[ORIENTATION_VERTICAL]. Style is one of @[TOOLBAR_BOTH], @[TOOLBAR_ICONS] and @[TOOLBAR_TEXT]
//!
//!

int get_button_relief( );
//!

GTK.Toolbar insert_item( string label, string tooltip, string prv, GTK.Widget icon, function clicked_cb, mixed clicked_arg, int position );
//! Arguments as for append_item, but an extra position argument at the end.
//! Adds a new button after the item at the specified position.
//!
//!

GTK.Toolbar insert_space( int pixels );
//! Inserts a small space at the specified postion.
//!
//!

GTK.Toolbar insert_widget( GTK.Widget widget, string tootip, string prv, int pos );
//! Insert a custom widgets.
//!
//!

GTK.Toolbar prepend_item( string label, string tooltip, string prv, GTK.Widget icon, function clicked_cb, mixed clicked_arg );
//! Arguments as for append_item
//! Adds a new button to the end of the toolbar.
//!
//!

GTK.Toolbar prepend_space( );
//! Adds a small space.
//!
//!

GTK.Toolbar prepend_widget( GTK.Widget widget, string tootip, string prv );
//! Prepend a custom widgets. Arguments are widget, tooltip, private
//!
//!

GTK.Toolbar set_button_relief( int relief );
//!

GTK.Toolbar set_orientation( int orientation );
//! Set the orientation, one of @[ORIENTATION_HORIZONTAL] and @[ORIENTATION_VERTICAL]
//!
//!

GTK.Toolbar set_space_size( int pixels );
//! Set the width (or height) of the space created by append_space.
//!
//!

GTK.Toolbar set_space_style( int style );
//!

GTK.Toolbar set_style( int style );
//! Set the style, one of @[TOOLBAR_BOTH], @[TOOLBAR_ICONS] and @[TOOLBAR_TEXT]
//!
//!

GTK.Toolbar set_tooltips( int tootipp );
//! If true, show the tooltips.
//!
//!
