//! A container that can only contain one child, and accepts events.
//! draws a bevelbox around itself.
//!@expr{ GTK1.Button("A button")@}
//!@xml{<image>../images/gtk1_button.png</image>@}
//!
//!@expr{ GTK1.Button("A button\nwith multiple lines\nof text")@}
//!@xml{<image>../images/gtk1_button_2.png</image>@}
//!
//!@expr{ GTK1.Button()->add(GTK1.Image(GDK1.Image(0)->set(Image.Image(100,40)->test())))@}
//!@xml{<image>../images/gtk1_button_3.png</image>@}
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

inherit GTK1.Container;

GTK1.Button clicked( );
//! Emulate a 'clicked' event (press followed by release).
//!
//!

protected GTK1.Button create( string|void label_text );
//! If a string is supplied, a W(Label) is created and added to the button.
//!
//!

GTK1.Button enter( );
//! Emulate a 'enter' event.
//!
//!

GTK1.Widget get_child( );
//! The (one and only) child of this container.
//!
//!

int get_relief( );
//! One of @[RELIEF_HALF], @[RELIEF_NONE] and @[RELIEF_NORMAL], set with set_relief()
//!
//!

GTK1.Button leave( );
//! Emulate a 'leave' event.
//!
//!

GTK1.Button pressed( );
//! Emulate a 'press' event.
//!
//!

GTK1.Button released( );
//! Emulate a 'release' event.
//!
//!

GTK1.Button set_relief( int newstyle );
//! One of @[RELIEF_HALF], @[RELIEF_NONE] and @[RELIEF_NORMAL]
//!
//!
