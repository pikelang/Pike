//! Use this widget when you want the user to input a single line of text.
//!@code{ GTK.Entry()->set_text("Hello world")->set_editable(1)@}
//!@xml{<image src='../images/gtk_entry.png'/>@}
//!
//!
//!
inherit Editable;

Entry append_text( string text )
//! Append the specified string at the end of the entry
//!
//!

static Entry create( )
//!

string get_text( )
//! Returns the contents of the entry widget.
//!
//!

Entry prepend_text( string text )
//! Prepend the specified string to the start of the entry
//!
//!

Entry set_max_length( int maxlen )
//! text is truncated if needed
//!
//!

Entry set_text( string text )
//! Set the text to the specified string. The old text is dropped.
//!
//!

Entry set_visibility( int visiblep )
//! 0 indicates invisible text (password boxes, as an example)
//!
//!
