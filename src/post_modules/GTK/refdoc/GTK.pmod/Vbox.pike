//! Most packing is done by creating boxes. These are invisible widget
//! containers that we can pack our widgets into which come in two
//! forms, a horizontal box, and a vertical box. This is the vertical
//! one.  When packing widgets into a vertical box, the objects are
//! inserted horizontally from top to bottom or bottom to top depending
//! on the call used.
//!
//!@expr{ GTK.Vbox(0,0)->add(GTK.Button("Hello"))->add(GTK.Button("World"))->pack_end_defaults(GTK.Button("From right"))->pack_start_defaults(GTK.Button("From left"))@}
//!@xml{<image>../images/gtk_vbox.png</image>@}
//!
//!@expr{ GTK.Vbox(1,0)->add(GTK.Button("Hello"))->add(GTK.Button("World"))->pack_end_defaults(GTK.Button("From right"))->pack_start_defaults(GTK.Button("From left"))@}
//!@xml{<image>../images/gtk_vbox_2.png</image>@}
//!
//!@expr{ GTK.Vbox(1,40)->add(GTK.Button("Hello"))->add(GTK.Button("World"))->pack_end_defaults(GTK.Button("From right"))->pack_start_defaults(GTK.Button("From left"))@}
//!@xml{<image>../images/gtk_vbox_3.png</image>@}
//!
//!
//!

inherit GTK.Box;

static GTK.Vbox create( int uniformp, int padding );
//! Create a new horizontal box widget.
//! If all_same_size is true, all widgets will have exactly the same size.
//! padding is added to the top and bottom of the children.
//!
//!
