//! A gamma curve widget.. Rather complex.
//! 
//! Lets the user edit a gamma curve (a one-to-one mapping usually used
//! to adjust the intensity of an image to the physical characteristics
//! of the output device). You can set the minimum and maximum values
//! for input and output. You can set the initial vector as well. You
//! are guaranteed that every input value will have a (not necessarily
//! unique) output value specified.
//! 
//!@expr{ GTK.GammaCurve()->set_usize(210,130)@}
//!@xml{<image>../images/gtk_gammacurve.png</image>@}
//!
//!
//!

inherit GTK.Vbox;

static GTK.GammaCurve create( );
//! Create a new gamma curve
//!
//!

GTK.Curve get_curve( );
//! The actual curve
//!
//!

float get_gamma( );
//! The current gamma value
//!
//!
