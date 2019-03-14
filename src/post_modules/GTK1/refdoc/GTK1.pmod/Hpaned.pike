//! The paned window widgets are useful when you want to divide an area
//! into two parts, with the relative size of the two parts controlled
//! by the user. A groove is drawn between the two portions with a
//! handle that the user can drag to change the ratio. This widgets
//! makes a horizontal division
//! 
//! See W(Paned) for details.
//! 
//!@expr{ GTK1.Hpaned()->add1(GTK1.Label("Left\nSide\nOf\nPane"))->add2(GTK1.Label("Right\nSide\nOf\nPane"))->set_usize(100,100)@}
//!@xml{<image>../images/gtk1_hpaned.png</image>@}
//!
//!
//!
//!

inherit GTK1.Paned;

protected GTK1.Hpaned create( );
//!
