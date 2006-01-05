//! Properties
//! int draw-as-radio
//!
//!
//!  Signals:
//! @b{toggled@}
//!

inherit GTK2.Action;

static GTK2.ToggleAction create( string|mapping name_or_props, string|void label, string|void tooltip, string|void stock_id );
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
