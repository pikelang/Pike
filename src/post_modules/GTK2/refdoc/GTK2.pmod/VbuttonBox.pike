//! A Vbutton_box is very similar to a Vbox.
//! The major diffference is that the button box
//! is made to pack buttons in, and has a few convenience function for
//! normal button layouts.
//!
//!@expr{ GTK2.VbuttonBox()->add(GTK2.Button("Hello"))->add(GTK2.Button("World"))->set_size_request(100,300)@}
//!@xml{<image>../images/gtk2_vbuttonbox.png</image>@}
//!
//!@expr{ GTK2.VbuttonBox()->add(GTK2.Button("Hello"))->add(GTK2.Button("World"))->set_layout(GTK2.BUTTONBOX_SPREAD)->set_size_request(100,300)@}
//!@xml{<image>../images/gtk2_vbuttonbox_2.png</image>@}
//!
//!@expr{ GTK2.VbuttonBox()->add(GTK2.Button("Hello"))->add(GTK2.Button("World"))->set_layout(GTK2.BUTTONBOX_EDGE)->set_size_request(100,300)@}
//!@xml{<image>../images/gtk2_vbuttonbox_3.png</image>@}
//!
//!@expr{ GTK2.VbuttonBox()->add(GTK2.Button("Hello"))->add(GTK2.Button("World"))->set_layout(GTK2.BUTTONBOX_START)->set_size_request(100,300)@}
//!@xml{<image>../images/gtk2_vbuttonbox_4.png</image>@}
//!
//!@expr{ GTK2.VbuttonBox()->add(GTK2.Button("Hello"))->add(GTK2.Button("World"))->set_layout(GTK2.BUTTONBOX_END)->set_size_request(100,300)@}
//!@xml{<image>../images/gtk2_vbuttonbox_5.png</image>@}
//!
//!
//!

inherit GTK2.ButtonBox;

static GTK2.VbuttonBox create( mapping|void props );
//! Create a new vertical button box
//!
//!
