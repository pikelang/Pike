//! A Vbutton_box is very similar to a Vbox.
//! The major diffference is that the button box
//! is made to pack buttons in, and has a few convenience function for
//! normal button layouts.
//!
//!@expr{ GTK.VbuttonBox()->add(GTK.Button("Hello"))->add(GTK.Button("World"))->set_usize(100,300)@}
//!@xml{<image>../images/gtk_vbuttonbox.png</image>@}
//!
//!@expr{ GTK.VbuttonBox()->add(GTK.Button("Hello"))->add(GTK.Button("World"))->set_layout(GTK.BUTTONBOX_SPREAD)->set_usize(100,300)@}
//!@xml{<image>../images/gtk_vbuttonbox_2.png</image>@}
//!
//!@expr{ GTK.VbuttonBox()->add(GTK.Button("Hello"))->add(GTK.Button("World"))->set_layout(GTK.BUTTONBOX_EDGE)->set_usize(100,300)@}
//!@xml{<image>../images/gtk_vbuttonbox_3.png</image>@}
//!
//!@expr{ GTK.VbuttonBox()->add(GTK.Button("Hello"))->add(GTK.Button("World"))->set_layout(GTK.BUTTONBOX_START)->set_usize(100,300)@}
//!@xml{<image>../images/gtk_vbuttonbox_4.png</image>@}
//!
//!@expr{ GTK.VbuttonBox()->add(GTK.Button("Hello"))->add(GTK.Button("World"))->set_layout(GTK.BUTTONBOX_END)->set_usize(100,300)@}
//!@xml{<image>../images/gtk_vbuttonbox_5.png</image>@}
//!
//!
//!

inherit GTK.ButtonBox;

static GTK.VbuttonBox create( );
//! Create a new vertical button box
//!
//!
