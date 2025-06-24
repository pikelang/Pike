// Automatically generated from "gtkwindowgroup.pre".
// Do NOT edit.

//! Limit the effect of grabs.
//!
//!

inherit G.Object;
//!

GTK2.WindowGroup add_window( GTK2.Window window );
//! Add a window.
//!
//!

protected void create( );
//! Creates a new GTK2.WindowGroup object.  Grabs added with GTK2.grab_add() 
//! only affect windows with the same GTK2.WindowGroup.
//!
//!

GTK2.WindowGroup remove_window( GTK2.Window window );
//! Remove a window.
//!
//!
