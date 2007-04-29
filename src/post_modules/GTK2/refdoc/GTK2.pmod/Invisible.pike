//! An invisible container, useful, eh? :)
//! Properties:
//! GDK2.Screen screen
//!
//!

inherit GTK2.Widget;

static GTK2.Invisible create( mapping|void props );
//! Create a new invisible widget
//!
//!

GTK2.GdkScreen get_screen( );
//! Gets the screen associated with this object.
//!
//!

GTK2.Invisible set_screen( GTK2.GdkScreen screen );
//! Sets the screen where this object will be displayed.
//!
//!
