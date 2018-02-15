//! A HbuttonBox is very similar to a Hbox.
//! The major diffference is that the button box
//! is made to pack buttons in, and has a few convenience function for
//! normal button layouts.
//!
//!@expr{ GTK1.HbuttonBox()->add(GTK1.Button("Hello"))->add(GTK1.Button("World"))->set_usize(400,30)@}
//!@xml{<image>../images/gtk1_hbuttonbox.png</image>@}
//!
//!@expr{ GTK1.HbuttonBox()->add(GTK1.Button("Hello"))->add(GTK1.Button("World"))->set_layout(GTK1.BUTTONBOX_SPREAD)->set_usize(400,30)@}
//!@xml{<image>../images/gtk1_hbuttonbox_2.png</image>@}
//!
//!@expr{ GTK1.HbuttonBox()->add(GTK1.Button("Hello"))->add(GTK1.Button("World"))->set_layout(GTK1.BUTTONBOX_EDGE)->set_usize(400,30)@}
//!@xml{<image>../images/gtk1_hbuttonbox_3.png</image>@}
//!
//!@expr{ GTK1.HbuttonBox()->add(GTK1.Button("Hello"))->add(GTK1.Button("World"))->set_layout(GTK1.BUTTONBOX_START)->set_usize(400,30)@}
//!@xml{<image>../images/gtk1_hbuttonbox_4.png</image>@}
//!
//!@expr{ GTK1.HbuttonBox()->add(GTK1.Button("Hello"))->add(GTK1.Button("World"))->set_layout(GTK1.BUTTONBOX_END)->set_usize(400,30)@}
//!@xml{<image>../images/gtk1_hbuttonbox_5.png</image>@}
//!
//!
//!

inherit GTK1.ButtonBox;

protected GTK1.HbuttonBox create( );
//! Create a new horizontal button box
//!
//!
