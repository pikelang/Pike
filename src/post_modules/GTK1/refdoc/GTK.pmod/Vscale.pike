//! The GTK.VScale widget is used to allow the user to select a value
//! using a vertical slider. A GtkAdjustment is used to set the initial
//! value, the lower and upper bounds, and the step and page
//! increments.
//! 
//! The position to show the current value, and the number of decimal
//! places shown can be set using the parent W(Scale) class's
//! functions.
//! 
//!@expr{ GTK.Vscale(GTK.Adjustment())->set_usize(30,100)@}
//!@xml{<image>../images/gtk_vscale.png</image>@}
//!
//! 
//!
//!

inherit GTK.Scale;

static GTK.Vscale create( GTK.Adjustment settings );
//! Used to create a new vscale widget.
//! The adjustment argument can either be an existing W(Adjustment), or
//! 0, in which case one will be created for you. Specifying 0 might
//! actually be useful in this case, if you wish to pass the newly
//! automatically created adjustment to the constructor function of
//! some other widget which will configure it for you, such as a text
//! widget.
//!
//!
