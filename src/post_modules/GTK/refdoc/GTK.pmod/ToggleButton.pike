//! Toggle buttons are derived from normal buttons and are very
//! similar, except they will always be in one of two states,
//! alternated by a click. They may be depressed, and when you click
//! again, they will pop back up. Click again, and they will pop back
//! down.
//! 
//!@code{ GTK.ToggleButton("Toggle button")@}
//!@xml{<image>../images/gtk_togglebutton.png</image>@}
//!
//!@code{ GTK.ToggleButton("Toggle button")->set_active( 1 )@}
//!@xml{<image>../images/gtk_togglebutton_2.png</image>@}
//!
//! 
//!
//!
//!  Signals:
//! @b{toggled@}
//!

inherit GTK.Button;

static GTK.ToggleButton create( string|void label );
//! If you supply a string, a label will be created and inserted in the button.
//! Otherwise, use -&gt;add(widget) to create the contents of the button.
//!
//!

int get_active( );
//! returns 1 if the button is pressed, 0 otherwise.
//!
//!

GTK.ToggleButton set_active( int activep );
//! If activep is true, the toggle button will be activated.
//!
//!

GTK.ToggleButton set_mode( int mode );
//! If true, draw indicator
//!
//!

GTK.ToggleButton toggled( );
//! emulate a 'toggle' of the button. This will emit a 'toggled' signal.
//!
//!
