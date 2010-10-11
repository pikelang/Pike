//! A label for accelerators.
//!
//!

inherit GTK.Label;

static GTK.AccelLabel create( string text );
//!

int get_accel_width( );
//!

int refetch( );
//!

GTK.AccelLabel set_accel_widget( GTK.Widget accel_widget );
//!
