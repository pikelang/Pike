//! Toggle buttons are derived from normal buttons and are very
//! similar, except they will always be in one of two states,
//! alternated by a click. They may be depressed, and when you click
//! again, they will pop back up. Click again, and they will pop back
//! down.
//! 
//!@expr{ GTK2.ToggleButton("Toggle button")@}
//!@xml{<image>../images/gtk2_togglebutton.png</image>@}
//!
//!@expr{ GTK2.ToggleButton("Toggle button")->set_active( 1 )@}
//!@xml{<image>../images/gtk2_togglebutton_2.png</image>@}
//!
//! 
//! Properties:
//! int active
//! int draw-indicator
//! int inconsistent
//!
//!
//!  Signals:
//! @b{toggled@}
//!

inherit GTK2.Button;

static GTK2.ToggleButton create( string|mapping label_or_props );
//! If you supply a string, a label will be created and inserted in the button.
//! Otherwise, use -&gt;add(widget) to create the contents of the button.
//!
//!

int get_active( );
//! Returns true if button is pressed, and false if it is raised.
//!
//!

int get_inconsistent( );
//! Gets the value set by set_inconsistent().
//!
//!

int get_mode( );
//! Retrieves whether the button is displayed as a separator indicator and
//! label.
//!
//!

GTK2.ToggleButton set_active( int activep );
//! If activep is true, the toggle button will be activated.
//!
//!

GTK2.ToggleButton set_inconsistent( int setting );
//! If the user has selected a range of elements (such as some text or
//! spreadsheet cells) that are affected by a toggle button, and the current
//! values in that range are inconsistent, you may want to display the toggle
//! in an "in between" state.  This function turns on "in between" display.
//!
//!

GTK2.ToggleButton set_mode( int mode );
//! If true, draw indicator
//!
//!

GTK2.ToggleButton toggled( );
//! emulate a 'toggle' of the button. This will emit a 'toggled' signal.
//!
//!
