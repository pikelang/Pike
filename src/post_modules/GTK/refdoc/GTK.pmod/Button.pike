//! A container that can only contain one child, and accepts events.
//! draws a bevelbox around itself.
//!@expr{ GTK.Button("A button")@}
//!@xml{<image>../images/gtk_button.png</image>@}
//!
//!@expr{ GTK.Button("A button\nwith multiple lines\nof text")@}
//!@xml{<image>../images/gtk_button_2.png</image>@}
//!
//!@expr{ GTK.Button()->add(GTK.Image(GDK.Image(0)->set(Image.image(100,40)->test())))@}
//!@xml{<image>../images/gtk_button_3.png</image>@}
//!
//!
//!
//!  Signals:
//! @b{clicked@}
//! Called when the button is pressed, and then released
//!
//!
//! @b{enter@}
//! Called when the mouse enters the button
//!
//!
//! @b{leave@}
//! Called when the mouse leaves the button
//!
//!
//! @b{pressed@}
//! Called when the button is pressed
//!
//!
//! @b{released@}
//! Called when the button is released
//!
//!

inherit GTK.Container;

GTK.Button clicked( );
//! Emulate a 'clicked' event (press followed by release).
//!
//!

static GTK.Button create( string|void label_text );
//! If a string is supplied, a W(Label) is created and added to the button.
//!
//!

GTK.Button enter( );
//! Emulate a 'enter' event.
//!
//!

GTK.Widget child( );
//! The (one and only) child of this container.
//!
//!

int get_relief( );
//! One of @[RELIEF_HALF], @[RELIEF_NONE] and @[RELIEF_NORMAL], set with set_relief()
//!
//!

GTK.Button leave( );
//! Emulate a 'leave' event.
//!
//!

GTK.Button pressed( );
//! Emulate a 'press' event.
//!
//!

GTK.Button released( );
//! Emulate a 'release' event.
//!
//!

GTK.Button set_relief( int newstyle );
//! One of @[RELIEF_HALF], @[RELIEF_NONE] and @[RELIEF_NORMAL]
//!
//!
