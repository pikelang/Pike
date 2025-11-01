// Automatically generated from "gtkmessagedialog.pre".
// Do NOT edit.

//! A dialog with an image representing the type of message (Error, Question).
//! alongside some message text.  It's simply a convenience widget; you could
//! construct the equivalent of GTK2.MessageDialog from GTK2.Dialog without too
//! much effort, but GTK2.MessageDialog saves typing.
//! Properties:
//! int buttons
//! GTK2.Widget image
//! int message-type
//! string secondary-text
//! 
//! Style properties:
//! int message-border
//! int use-separator
//!
//!

inherit GTK2.Dialog;
//!

protected void create( void flags, void type, void buttons, void message, void parent );
//! Creates a new message dialog, which is a simple dialog with an icon
//! indicating the dialog type (error, warning, etc) and some text the user
//! may want to see.  When the user clicks a button a "response" signal is
//! emitted with response IDs from @[RESPONSE_ACCEPT], @[RESPONSE_APPLY], @[RESPONSE_CANCEL], @[RESPONSE_CLOSE], @[RESPONSE_DELETE_EVENT], @[RESPONSE_HELP], @[RESPONSE_NO], @[RESPONSE_NONE], @[RESPONSE_OK], @[RESPONSE_REJECT] and @[RESPONSE_YES].  See
//! GTK2.Dialog for more details.
//!
//!

GTK2.MessageDialog format_secondary_markup( string text );
//! Sets the secondary text of the message dialog to be text, which is
//! marked up with the Pango text markup language.
//! 
//! Note tha tsetting a secondary text makes the primary text become bold,
//! unless you have provided explicit markup.
//!
//!

GTK2.MessageDialog format_secondary_text( string text );
//! Sets the secondary text of the message dialog to be text.
//! 
//! Note that setting a secondary text makes the primary text become bold,
//! unless you have provided explicit markup.
//!
//!

GTK2.MessageDialog set_image( GTK2.Widget image );
//! Sets the dialog's image to image.
//!
//!

GTK2.MessageDialog set_markup( string text );
//! Sets the text of the message dialog to be text, which is marked up with
//! the Pango text markup language.
//!
//!
