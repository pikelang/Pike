//! Limit the effect of grabs.
//!
//!

inherit G.Object;

GTK2.WindowGroup add_window( GTK2.Window window );
//! Add a window.
//!
//!

static GTK2.WindowGroup create( );
//! Creates a new GTK2.WindowGroup object.  Grabs added with GTK2.grab_add() only
//! affect windows with the same GTk.WindowGroup.
//!
//!

GTK2.WindowGroup remove_window( GTK2.Window window );
//! Remove a window.
//!
//!
