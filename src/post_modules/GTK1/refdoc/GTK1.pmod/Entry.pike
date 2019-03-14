//! Use this widget when you want the user to input a single line of text.
//!@expr{ GTK1.Entry()->set_text("Hello world")->set_editable(1)@}
//!@xml{<image>../images/gtk1_entry.png</image>@}
//!
//!
//!

inherit GTK1.Editable;

GTK1.Entry append_text( string text );
//! Append the specified string at the end of the entry
//!
//!

protected GTK1.Entry create( );
//!

string get_text( );
//! Returns the contents of the entry widget.
//!
//!

GTK1.Entry prepend_text( string text );
//! Prepend the specified string to the start of the entry
//!
//!

GTK1.Entry set_max_length( int maxlen );
//! text is truncated if needed
//!
//!

GTK1.Entry set_text( string text );
//! Set the text to the specified string. The old text is dropped.
//!
//!

GTK1.Entry set_visibility( int visiblep );
//! 0 indicates invisible text (password boxes, as an example)
//!
//!
