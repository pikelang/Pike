// Automatically generated from "gtktoggletoolbutton.pre".
// Do NOT edit.

//! A ToolItem containing a toggle button.
//!
//!
//!  Signals:
//! @b{toggled@}
//!

inherit GTK2.ToolButton;
//!

protected void create( void stock_id );
//! Creates a new toggle tool button, with or without
//! a stock id.
//!
//!

int get_active( );
//! Returns the status of the toggle tool button, true if it
//! is pressed in and false if it isn't.
//!
//!

GTK2.ToggleToolButton set_active( int is_active );
//! Sets the status of the toggle tool button.  Set to true
//! if you want the button to be pressed in, and false to
//! raise it.  This causes the toggled signal to be emitted.
//!
//!
