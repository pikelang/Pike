//! Properties
//! GTK2.RadioAction group
//! int value
//!
//!
//!  Signals:
//! @b{changed@}
//!

inherit GTK2.ToggleAction;

static GTK2.RadioAction create( string|mapping name_or_props, string|void label, string|void tooltip, string|void stock_id, int|void value );
//! Creates a new GTK2.ToggleAction object.
//!
//!

int get_current_value( );
//! Obtains the value property of the currently active member.
//!
//!

array get_group( );
//! Returns the list representing the radio group.
//!
//!

GTK2.RadioAction set_group( GTK2.RadioAction member );
//! Sets the radio group.
//!
//!
