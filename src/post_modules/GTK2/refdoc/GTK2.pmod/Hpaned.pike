//! The paned window widgets are useful when you want to divide an area
//! into two parts, with the relative size of the two parts controlled
//! by the user. A groove is drawn between the two portions with a
//! handle that the user can drag to change the ratio. This widgets
//! makes a horizontal division
//! 
//! See W(Paned) for details.
//! 
//!@expr{ GTK2.Hpaned()->add1(GTK2.Label("Left\nSide\nOf\nPane"))->add2(GTK2.Label("Right\nSide\nOf\nPane"))->set_size_request(100,100)@}
//!@xml{<image>../images/gtk2_hpaned.png</image>@}
//!
//!
//!
//!

inherit GTK2.Paned;

static GTK2.Hpaned create( mapping|void props );
//! Create a new W(Hpaned) widget.
//!
//!
