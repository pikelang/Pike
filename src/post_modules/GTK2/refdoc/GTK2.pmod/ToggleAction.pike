// Automatically generated from "gtktoggleaction.pre".
// Do NOT edit.

//! Properties
//! int active
//! int draw-as-radio
//!
//!
//!  Signals:
//! @b{toggled@}
//!

inherit GTK2.Action;
//!

protected void create( void name_or_props, void label, void tooltip, void stock_id );
//! Creates a new GTK2.ToggleAction object.
//!
//!

int get_active( );
//! Returns the checked state.
//!
//!

int get_draw_as_radio( );
//! Returns whether the action should have proxies like a radio action.
//!
//!

GTK2.ToggleAction set_active( int setting );
//! Sets the checked state.
//!
//!

GTK2.ToggleAction set_draw_as_radio( int setting );
//! Sets whether the action should have proxies like a radio action.
//!
//!

GTK2.ToggleAction toggled( );
//! Emits the "toggled" signal.
//!
//!
