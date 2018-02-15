//! A Vbutton_box is very similar to a Vbox.
//! The major diffference is that the button box
//! is made to pack buttons in, and has a few convenience function for
//! normal button layouts.
//!
//!@expr{ GTK1.VbuttonBox()->add(GTK1.Button("Hello"))->add(GTK1.Button("World"))->set_usize(100,300)@}
//!@xml{<image>../images/gtk1_vbuttonbox.png</image>@}
//!
//!@expr{ GTK1.VbuttonBox()->add(GTK1.Button("Hello"))->add(GTK1.Button("World"))->set_layout(GTK1.BUTTONBOX_SPREAD)->set_usize(100,300)@}
//!@xml{<image>../images/gtk1_vbuttonbox_2.png</image>@}
//!
//!@expr{ GTK1.VbuttonBox()->add(GTK1.Button("Hello"))->add(GTK1.Button("World"))->set_layout(GTK1.BUTTONBOX_EDGE)->set_usize(100,300)@}
//!@xml{<image>../images/gtk1_vbuttonbox_3.png</image>@}
//!
//!@expr{ GTK1.VbuttonBox()->add(GTK1.Button("Hello"))->add(GTK1.Button("World"))->set_layout(GTK1.BUTTONBOX_START)->set_usize(100,300)@}
//!@xml{<image>../images/gtk1_vbuttonbox_4.png</image>@}
//!
//!@expr{ GTK1.VbuttonBox()->add(GTK1.Button("Hello"))->add(GTK1.Button("World"))->set_layout(GTK1.BUTTONBOX_END)->set_usize(100,300)@}
//!@xml{<image>../images/gtk1_vbuttonbox_5.png</image>@}
//!
//!
//!

inherit GTK1.ButtonBox;

protected GTK1.VbuttonBox create( );
//! Create a new vertical button box
//!
//!
