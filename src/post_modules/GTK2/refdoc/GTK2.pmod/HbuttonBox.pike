//! A HbuttonBox is very similar to a Hbox.
//! The major diffference is that the button box
//! is made to pack buttons in, and has a few convenience function for
//! normal button layouts.
//!
//!@expr{ GTK2.HbuttonBox()->add(GTK2.Button("Hello"))->add(GTK2.Button("World"))->set_size_request(400,30)@}
//!@xml{<image>../images/gtk2_hbuttonbox.png</image>@}
//!
//!@expr{ GTK2.HbuttonBox()->add(GTK2.Button("Hello"))->add(GTK2.Button("World"))->set_layout(GTK2.BUTTONBOX_SPREAD)->set_size_request(400,30)@}
//!@xml{<image>../images/gtk2_hbuttonbox_2.png</image>@}
//!
//!@expr{ GTK2.HbuttonBox()->add(GTK2.Button("Hello"))->add(GTK2.Button("World"))->set_layout(GTK2.BUTTONBOX_EDGE)->set_size_request(400,30)@}
//!@xml{<image>../images/gtk2_hbuttonbox_3.png</image>@}
//!
//!@expr{ GTK2.HbuttonBox()->add(GTK2.Button("Hello"))->add(GTK2.Button("World"))->set_layout(GTK2.BUTTONBOX_START)->set_size_request(400,30)@}
//!@xml{<image>../images/gtk2_hbuttonbox_4.png</image>@}
//!
//!@expr{ GTK2.HbuttonBox()->add(GTK2.Button("Hello"))->add(GTK2.Button("World"))->set_layout(GTK2.BUTTONBOX_END)->set_size_request(400,30)@}
//!@xml{<image>../images/gtk2_hbuttonbox_5.png</image>@}
//!
//!
//!

inherit GTK2.ButtonBox;

static GTK2.HbuttonBox create( mapping|void props );
//! Create a new horizontal button box
//!
//!
