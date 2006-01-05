//! A label for accelerators.
//! Properties:
//! GTK2.Widget accel-widget
//!
//!

inherit GTK2.Label;

static GTK2.AccelLabel create( string|mapping text_or_props );
//! Creates a new W(AccelLabel).
//!
//!

GTK2.Widget get_accel_widget( );
//! Fetches the widget monitored by this accelerator label.
//!
//!

int get_accel_width( );
//! Returns the width needed to display the accelerator key(s).  This is used
//! by menus to align all of the W(MenuItem).
//!
//!

int refetch( );
//! Recreates the string representing the accelerator keys.  This should not
//! be needed since the string is automatically updated whenever accelerators
//! are added or removed from the associated widget.
//!
//!

GTK2.AccelLabel set_accel_widget( GTK2.Widget accel_widget );
//! Sets the widget to be monitored.
//!
//!
