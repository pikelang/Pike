// Automatically generated from "gtkvscale.pre".
// Do NOT edit.

//! The GTK2.HScale widget is used to allow the user to select a value
//! using a horizontal slider. A GTK2.Adjustment is used to set the
//! initial value, the lower and upper bounds, and the step and page
//! increments.
//! 
//! See W(Scale) for details
//! 
//! The position to show the current value, and the number of decimal
//! places shown can be set using the parent W(Scale) class's
//! functions.
//! 
//!@expr{ GTK2.Vscale(GTK2.Adjustment())->set_size_request(300,30)@}
//!@xml{<image>../images/gtk2_vscale.png</image>@}
//!
//!
//!

inherit GTK2.Scale;
//!

protected void create( void settings_or_min_props, void max, void step );
//! Used to create a new hscale widget.
//! Either pass an W(Adjustment), or three floats representing min, max, and
//! step values.
//!
//!
