//! The paned window widgets are useful when you want to divide an area
//! into two parts, with the relative size of the two parts controlled
//! by the user. A groove is drawn between the two portions with a
//! handle that the user can drag to change the ratio. This widgets
//! makes a vertical division
//!
//!@expr{ GTK1.Vpaned()->add1(GTK1.Label("Top Side Of Pane"))->add2(GTK1.Label("Bottom"))->set_usize(100,100)@}
//!@xml{<image>../images/gtk1_vpaned.png</image>@}
//!
//!
//!

inherit GTK1.Paned;

protected GTK1.Vpaned create( );
//!
