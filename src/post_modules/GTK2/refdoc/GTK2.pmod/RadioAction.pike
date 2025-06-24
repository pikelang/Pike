// Automatically generated from "gtkradioaction.pre".
// Do NOT edit.

//! Properties
//! int current-value
//! GTK2.RadioAction group
//! int value
//!
//!
//!  Signals:
//! @b{changed@}
//!

inherit GTK2.ToggleAction;
//!

protected void create( void name_or_props, void label, void tooltip, void stock_id, void value );
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

GTK2.RadioAction set_current_value( int value );
//! Sets the currently active group member to the member with value property
//! value.
//!
//!

GTK2.RadioAction set_group( GTK2.RadioAction member );
//! Sets the radio group.
//!
//!
