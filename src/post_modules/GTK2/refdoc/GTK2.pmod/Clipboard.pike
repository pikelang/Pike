//! Clipboard implementation.
//!
//!

inherit G.Object;

GTK2.Clipboard clear( );
//! Clears the contents of the clipboard.
//!
//!

GTK2.Clipboard destroy( );
//! Destructor.
//!
//!

GTK2.Clipboard get( GDK2.Atom selection );
//! Returns the clipboard object for the given selection.
//!
//!

GTK2.GdkDisplay get_display( );
//! Gets the GDK2.Display associated with this clipboard.
//!
//!

GTK2.Clipboard get_for_display( GTK2.GdkDisplay display, GDK2.Atom selection );
//! Returns the clipboard object for the given selection.
//!
//!

GTK2.Clipboard set_image( GTK2.GdkPixbuf pixbuf );
//! Sets the contents of the clipboard to the given GDK2(Pixbuf).
//!
//!

GTK2.Clipboard set_text( string text, int len );
//! Sets the contents of the clipboard to the given UTF-8 string.
//!
//!

GTK2.GdkPixbuf wait_for_image( );
//! Requests the contents of the clipboard as image and converts the result
//! to a GDK2.Pixbuf.
//!
//!

string wait_for_text( );
//! Requests the contents of the clipboard as text and converts the result
//! to UTF-8 if necessary.
//!
//!

int wait_is_image_available( );
//! Test to see if tehre is an image available to be pasted.
//!
//!

int wait_is_text_available( );
//! Test to see if there is text available to be pasted.  This is done by
//! requesting the TARGETS atom and checking if it contains any of the
//! supported text targets.
//!
//!
