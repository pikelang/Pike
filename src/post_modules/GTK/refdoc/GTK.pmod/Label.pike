//! A simple text label.
//!@code{ GTK.Label("A simple text label")@}
//!@xml{<image>../images/gtk_label.png</image>@}
//!
//!@code{ GTK.Label("Multi\nline text\nlabel here")@}
//!@xml{<image>../images/gtk_label_2.png</image>@}
//!
//!@code{ GTK.Label("Multi\nline text\nlabel here")->set_justify(GTK.JUSTIFY_LEFT)@}
//!@xml{<image>../images/gtk_label_3.png</image>@}
//!
//!@code{ GTK.Label("Multi\nline text\nlabel here")->set_justify(GTK.JUSTIFY_RIGHT)@}
//!@xml{<image>../images/gtk_label_4.png</image>@}
//!
//!
//!

inherit GTK.Misc;

static GTK.Label create( string text );
//! Creates a new label.
//!
//!

int parse_uline( string uline_string );
//! Convenience function to set the text and pattern by parsing
//! a string with embedded underscores, returns the appropriate
//! key symbol for the accelerator.
//!
//!

GTK.Label set( string text );
//! @b{DEPRECATED@} Compatibility function to set the text in the label. Use
//! set_text. This function can dissapear in the future.
//!
//!

GTK.Label set_justify( int justify );
//! one of @[JUSTIFY_CENTER], @[JUSTIFY_FILL], @[JUSTIFY_LEFT] and @[JUSTIFY_RIGHT]
//!
//!

GTK.Label set_line_wrap( int wrapp );
//! Should the label autolinewrap?
//!
//!

GTK.Label set_pattern( string pattern_string );
//! A string with either spaces or underscores.
//! It should be of the same length as the text.
//! 
//! When a character in the text has a matching _ in the pattern, the
//! character in the label will be underlined.
//! 
//!
//!

GTK.Label set_text( string text );
//! Set the text in the label
//!
//!
