//! Tooltips are the messages that appear next to a widget when the
//! mouse pointer is held over it for a short amount of time. They are
//! especially helpful for adding more verbose descriptions of things
//! such as buttons in a toolbar.
//! 
//! An individual tooltip belongs to a group of tooltips. A group is
//! created with a call to GTK.Tooltips(). Every tooltip in the
//! group can then be turned off with a call to disable()
//! and enabled with enable().
//! 
//! The length of time the user must keep the mouse over a widget
//! before the tip is shown, can be altered with set_delay(). This is
//! set on a 'per group of tooltips' basis.
//! 
//! To assign a tip to a particular W(Widget), set_tip() is used.
//! 
//! The default appearance of all tooltips in a program is determined
//! by the current gtk theme that the user has selected. To change the
//! tooltip appearance manually, use set_colors(). Again, this is per
//! group of tooltips.
//!
//!

inherit Data;

static Tooltips create( );
//! Creates an empty group of tooltips. This function initialises a
//! GTK.Tooltips structure. Without at least one such structure, you
//! can not add tips to your application.
//!
//!

Tooltips disable( );
//! Disable this tooltip collection
//!
//!

Tooltips enable( );
//! Enable this tooltip collection
//!
//!

Tooltips force_window( );
//! Realize the tooltip window (as returned from get_gdkwindow())
//!
//!

Tooltips set_colors( GDK.Color foreground, GDK.Color background );
//! Changes the foreground and background colors.
//!
//!

Tooltips set_delay( int delay );
//! Set the delat (in seconds)
//!
//!

Tooltips set_tip( GTK.Widget in, string to );
//! Adds a tooltip containing the message tip_text to the specified
//! W(Widget).
//!
//!
