//! Tooltips are the messages that appear next to a widget when the
//! mouse pointer is held over it for a short amount of time. They are
//! especially helpful for adding more verbose descriptions of things
//! such as buttons in a toolbar.
//! 
//! An individual tooltip belongs to a group of tooltips. A group is
//! created with a call to GTK2.Tooltips(). Every tooltip in the
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

inherit GTK2.Data;

static GTK2.Tooltips create( );
//! Creates an empty group of tooltips. This function initialises a
//! GTK2.Tooltips structure. Without at least one such structure, you
//! can not add tips to your application.
//!
//!

GTK2.Tooltips disable( );
//! Disable this tooltip collection.
//!
//!

GTK2.Tooltips enable( );
//! Enable this tooltip collection.
//!
//!

GTK2.Tooltips force_window( );
//! Realize the tooltip window (as returned from get_gdkwindow())
//!
//!

GTK2.Tooltips set_tip( GTK2.Widget in, string to );
//! Adds a tooltip containing the message tip_text to the specified
//! W(Widget).
//!
//!
