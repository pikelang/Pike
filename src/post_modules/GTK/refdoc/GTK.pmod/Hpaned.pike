//! The paned window widgets are useful when you want to divide an area
//! into two parts, with the relative size of the two parts controlled
//! by the user. A groove is drawn between the two portions with a
//! handle that the user can drag to change the ratio. This widgets
//! makes a horizontal division
//! 
//! See W(Paned) for details.
//! 
//!@expr{ GTK.Hpaned()->add1(GTK.Label("Left\nSide\nOf\nPane"))->add2(GTK.Label("Right\nSide\nOf\nPane"))->set_usize(100,100)@}
//!@xml{<image>../images/gtk_hpaned.png</image>@}
//!
//!
//!
//!

inherit GTK.Paned;

static GTK.Hpaned create( );
//!
