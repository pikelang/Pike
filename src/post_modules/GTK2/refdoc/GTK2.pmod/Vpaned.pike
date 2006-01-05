//! The paned window widgets are useful when you want to divide an area
//! into two parts, with the relative size of the two parts controlled
//! by the user. A groove is drawn between the two portions with a
//! handle that the user can drag to change the ratio. This widgets
//! makes a vertical division
//!
//!@expr{ GTK2.Vpaned()->add1(GTK2.Label("Top Side Of Pane"))->add2(GTK2.Label("Bottom"))->set_size_request(100,100)@}
//!@xml{<image>../images/gtk2_vpaned.png</image>@}
//!
//!
//!

inherit GTK2.Paned;

static GTK2.Vpaned create( mapping|void props );
//! Create a new W(Vpaned) widget.
//!
//!
