//! Most packing is done by creating boxes. These are invisible widget
//! containers that we can pack our widgets into which come in two
//! forms, a horizontal box, and a vertical box. This is the horizontal
//! one.  When packing widgets into a horizontal box, the objects are
//! inserted horizontally from left to right or right to left depending
//! on the call used.
//!
//!@expr{ GTK.Hbox(0,0)->add(GTK.Button("Hello"))->add(GTK.Button("World"))->pack_end_defaults(GTK.Button("From right"))->pack_start_defaults(GTK.Button("From left"))@}
//!@xml{<image>../images/gtk_hbox.png</image>@}
//!
//!@expr{ GTK.Hbox(1,0)->add(GTK.Button("Hello"))->add(GTK.Button("World"))->pack_end_defaults(GTK.Button("From right"))->pack_start_defaults(GTK.Button("From left"))@}
//!@xml{<image>../images/gtk_hbox_2.png</image>@}
//!
//!@expr{ GTK.Hbox(1,40)->add(GTK.Button("Hello"))->add(GTK.Button("World"))->pack_end_defaults(GTK.Button("From right"))->pack_start_defaults(GTK.Button("From left"))@}
//!@xml{<image>../images/gtk_hbox_3.png</image>@}
//!
//!
//!

inherit GTK.Box;

static GTK.Hbox create( int all_same_size, int hpadding );
//! Create a new horizontal box widget.
//! If all_same_size is true, all widgets will have exactly the same size.
//! hpadding is added to the left and right of the children.
//!
//!
