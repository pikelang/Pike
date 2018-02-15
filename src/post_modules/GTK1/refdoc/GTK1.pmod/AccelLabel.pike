//! A label for accelerators.
//!
//!

inherit GTK1.Label;

protected GTK1.AccelLabel create( string text );
//!

int get_accel_width( );
//!

int refetch( );
//!

GTK1.AccelLabel set_accel_widget( GTK1.Widget accel_widget );
//!
