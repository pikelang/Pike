//! A dialog is a window with a few default widgets added.
//! The 'vbox' is the main content area of the widget. The
//! 'action_area' is allocated for buttons (ok, cancel etc)
//!
//!
inherit Window;

static Dialog create( )
//! Create a new dialog widget.
//!
//!

GTK.HbuttonBox get_action_area( )
//! The action area, this is where the buttons (ok, cancel etc) go
//!
//!

GTK.Vbox get_vbox( )
//! The vertical box that should contain the contents of the dialog
//!
//!
