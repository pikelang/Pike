// Automatically generated from "gtkvbox.pre".
// Do NOT edit.

//! Most packing is done by creating boxes. These are invisible widget
//! containers that we can pack our widgets into which come in two
//! forms, a horizontal box, and a vertical box. This is the vertical
//! one.  When packing widgets into a vertical box, the objects are
//! inserted horizontally from top to bottom or bottom to top depending
//! on the call used.
//!
//!@expr{ GTK2.Vbox(0,0)->add(GTK2.Button("Hello"))->add(GTK2.Button("World"))->pack_end_defaults(GTK2.Button("From right"))->pack_start_defaults(GTK2.Button("From left"))@}
//!@xml{<image>../images/gtk2_vbox.png</image>@}
//!
//!@expr{ GTK2.Vbox(1,0)->add(GTK2.Button("Hello"))->add(GTK2.Button("World"))->pack_end_defaults(GTK2.Button("From right"))->pack_start_defaults(GTK2.Button("From left"))@}
//!@xml{<image>../images/gtk2_vbox_2.png</image>@}
//!
//!@expr{ GTK2.Vbox(1,40)->add(GTK2.Button("Hello"))->add(GTK2.Button("World"))->pack_end_defaults(GTK2.Button("From right"))->pack_start_defaults(GTK2.Button("From left"))@}
//!@xml{<image>../images/gtk2_vbox_3.png</image>@}
//!
//!
//!

inherit GTK2.Box;
//!

protected void create( void uniformp_or_props, void padding );
//! Create a new vertical box widget.
//! If all_same_size is true, all widgets will have exactly the same size.
//! padding is added to the top and bottom of the children.
//!
//!
