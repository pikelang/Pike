//! Use this widget when you want the user to input a single line of text.
//!@expr{ GTK.Entry()->set_text("Hello world")->set_editable(1)@}
//!@xml{<image>../images/gtk_entry.png</image>@}
//!
//!
//!

inherit GTK.Editable;

GTK.Entry append_text( string text );
//! Append the specified string at the end of the entry
//!
//!

static GTK.Entry create( );
//!

string get_text( );
//! Returns the contents of the entry widget.
//!
//!

GTK.Entry prepend_text( string text );
//! Prepend the specified string to the start of the entry
//!
//!

GTK.Entry set_max_length( int maxlen );
//! text is truncated if needed
//!
//!

GTK.Entry set_text( string text );
//! Set the text to the specified string. The old text is dropped.
//!
//!

GTK.Entry set_visibility( int visiblep );
//! 0 indicates invisible text (password boxes, as an example)
//!
//!
