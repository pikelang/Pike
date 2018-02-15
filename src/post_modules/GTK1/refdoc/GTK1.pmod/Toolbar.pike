//! Toolbars are usually used to group some number of widgets in order
//! to simplify customization of their look and layout. Typically a
//! toolbar consists of buttons with icons, labels and tooltips, but
//! any other widget can also be put inside a toolbar. Finally, items
//! can be arranged horizontally or vertically and buttons can be
//! displayed with icons, labels, or both.
//! 
//! Examples:
//!@expr{ lambda(){object t=GTK1.Toolbar( GTK1.ORIENTATION_HORIZONTAL, GTK1.TOOLBAR_TEXT );t->append_item( "Button 1", "Tooltip 1", "", GTK1.Frame(), lambda(){},0);t->append_space();t->append_item( "Button 2", "Tooltip 2", "", GTK1.Frame(), lambda(){},0);t->append_item( "Button 3", "Tooltip 3", "", GTK1.Frame(), lambda(){},0);t->append_space();t->append_item( "Button 4", "Tooltip 4", "", GTK1.Frame(), lambda(){},0);t->append_item( "Button 5", "Tooltip 5", "", GTK1.Frame(), lambda(){},0);return t;}()@}
//!@xml{<image>../images/gtk1_toolbar.png</image>@}
//!
//!@expr{ lambda(){object t=GTK1.Toolbar( GTK1.ORIENTATION_VERTICAL, GTK1.TOOLBAR_TEXT );t->append_item( "Button 1", "Tooltip 1", "", GTK1.Frame(), lambda(){},0);t->append_space();t->append_item( "Button 2", "Tooltip 2", "", GTK1.Frame(), lambda(){},0);t->append_item( "Button 3", "Tooltip 3", "", GTK1.Frame(), lambda(){},0);t->append_space();t->append_item( "Button 4", "Tooltip 4", "", GTK1.Frame(), lambda(){},0);t->append_item( "Button 5", "Tooltip 5", "", GTK1.Frame(), lambda(){},0);return t;}()@}
//!@xml{<image>../images/gtk1_toolbar_2.png</image>@}
//!
//!@expr{ lambda(){object i=GDK1.Image()->set(Image.Image(20,20)->test());object t=GTK1.Toolbar( GTK1.ORIENTATION_HORIZONTAL, GTK1.TOOLBAR_BOTH );t->append_item( "Button 1", "Tooltip 1", "", GTK1.Image(i), lambda(){},0);t->append_space();t->append_item( "Button 2", "Tooltip 2", "", GTK1.Image(i), lambda(){},0);t->append_item( "Button 3", "Tooltip 3", "", GTK1.Image(i), lambda(){},0);t->append_space();t->append_item( "Button 4", "Tooltip 4", "", GTK1.Image(i), lambda(){},0);t->append_item( "Button 5", "Tooltip 5", "", GTK1.Image(i), lambda(){},0);return t;}()@}
//!@xml{<image>../images/gtk1_toolbar_3.png</image>@}
//!
//! 
//!
//!
//!  Signals:
//! @b{orientation_changed@}
//!
//! @b{style_changed@}
//!

inherit GTK1.Container;

GTK1.Toolbar append_item( string label, string tooltip, string prv, GTK1.Widget icon, function clicked_cb, mixed clicked_arg );
//! Adds a new button to the start of the toolbar.
//!
//!

GTK1.Toolbar append_space( );
//! Adds a small space.
//!
//!

GTK1.Toolbar append_widget( GTK1.Widget widget, string tootip, string prv );
//! Append a custom widgets. Arguments are widget, tooltip, private
//!
//!

protected GTK1.Toolbar create( int orientation, int style );
//! Orientation is one of
//! @[ORIENTATION_HORIZONTAL] and @[ORIENTATION_VERTICAL]. Style is one of @[TOOLBAR_BOTH], @[TOOLBAR_ICONS] and @[TOOLBAR_TEXT]
//!
//!

int get_button_relief( );
//!

GTK1.Toolbar insert_item( string label, string tooltip, string prv, GTK1.Widget icon, function clicked_cb, mixed clicked_arg, int position );
//! Arguments as for append_item, but an extra position argument at the end.
//! Adds a new button after the item at the specified position.
//!
//!

GTK1.Toolbar insert_space( int pixels );
//! Inserts a small space at the specified postion.
//!
//!

GTK1.Toolbar insert_widget( GTK1.Widget widget, string tootip, string prv, int pos );
//! Insert a custom widgets.
//!
//!

GTK1.Toolbar prepend_item( string label, string tooltip, string prv, GTK1.Widget icon, function clicked_cb, mixed clicked_arg );
//! Arguments as for append_item
//! Adds a new button to the end of the toolbar.
//!
//!

GTK1.Toolbar prepend_space( );
//! Adds a small space.
//!
//!

GTK1.Toolbar prepend_widget( GTK1.Widget widget, string tootip, string prv );
//! Prepend a custom widgets. Arguments are widget, tooltip, private
//!
//!

GTK1.Toolbar set_button_relief( int relief );
//!

GTK1.Toolbar set_orientation( int orientation );
//! Set the orientation, one of @[ORIENTATION_HORIZONTAL] and @[ORIENTATION_VERTICAL]
//!
//!

GTK1.Toolbar set_space_size( int pixels );
//! Set the width (or height) of the space created by append_space.
//!
//!

GTK1.Toolbar set_space_style( int style );
//!

GTK1.Toolbar set_style( int style );
//! Set the style, one of @[TOOLBAR_BOTH], @[TOOLBAR_ICONS] and @[TOOLBAR_TEXT]
//!
//!

GTK1.Toolbar set_tooltips( int tootipp );
//! If true, show the tooltips.
//!
//!
