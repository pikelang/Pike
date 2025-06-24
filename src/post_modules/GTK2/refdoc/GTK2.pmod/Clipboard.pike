// Automatically generated from "gtkclipboard.pre".
// Do NOT edit.

//! Clipboard implementation.
//!
//!

inherit G.Object;
//!


GTK2.Clipboard clear( );
//! Clears the contents of the clipboard.
//!
//!

GTK2.Clipboard get( GDK2.Atom selection );
//! Returns the clipboard object for the given selection.
//!
//!

GDK2.Display get_display( );
//! Gets the GDK2.Display associated with this clipboard.
//!
//!

GTK2.Clipboard get_for_display( GDK2.Display display, GDK2.Atom selection );
//! Returns the clipboard object for the given selection.
//!
//!

GTK2.Clipboard set_image( GDK2.Pixbuf pixbuf );
//! Sets the contents of the clipboard to the given GDK2(Pixbuf).
//!
//!

GTK2.Clipboard set_text( sprintf_format text, sprintf_args... fmt );
//! Sets the contents of the clipboard to the given string.
//! If multiple arguments are supplied, sprintf() is called implicitly.
//!
//!

GDK2.Pixbuf wait_for_image( );
//! Requests the contents of the clipboard as image and converts the result
//! to a GDK2.Pixbuf.
//!
//!

string wait_for_rich_text( GTK2.TextBuffer buffer );
//! Requests the contents of the clipboard as rich text.
//!
//!

string wait_for_text( );
//! Requests the contents of the clipboard as text
//!
//!

int wait_is_image_available( );
//! Test to see if tehre is an image available to be pasted.
//!
//!

int wait_is_rich_text_available( GTK2.TextBuffer buffer );
//! Test to see if there is rich text available to be pasted.
//!
//!

int wait_is_text_available( );
//! Test to see if there is text available to be pasted.  This is done by
//! requesting the TARGETS atom and checking if it contains any of the
//! supported text targets.
//!
//!
