//! Toggle buttons are derived from normal buttons and are very
//! similar, except they will always be in one of two states,
//! alternated by a click. They may be depressed, and when you click
//! again, they will pop back up. Click again, and they will pop back
//! down.
//! 
//!@code{ GTK.ToggleButton("Toggle button")@}
//!@xml{<image src='../images/gtk_togglebutton.png'/>@}
//!
//!@code{ GTK.ToggleButton("Toggle button")->set_active( 1 )@}
//!@xml{<image src='../images/gtk_togglebutton_2.png'/>@}
//!
//! 
//!
//!
//!  Signals:
//! @b{toggled@}
//!

inherit Button;

static ToggleButton create( string|void label );
//! If you supply a string, a label will be created and inserted in the button.
//! Otherwise, use -&gt;add(widget) to create the contents of the button.
//!
//!

int get_active( );
//! returns 1 if the button is pressed, 0 otherwise.
//!
//!

ToggleButton set_active( int activep );
//! If activep is true, the toggle button will be activated.
//!
//!

ToggleButton set_mode( int mode );
//! If true, draw indicator
//!
//!

ToggleButton toggled( );
//! emulate a 'toggle' of the button. This will emit a 'toggled' signal.
//!
//!
