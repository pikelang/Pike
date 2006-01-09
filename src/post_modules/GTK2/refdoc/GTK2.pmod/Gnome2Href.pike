//! This widget is a GtkButton button that contains a URL. When clicked
//! it invokes the configured browser for the URL you provided.
//! 
//!@expr{ GTK2.Gnome2Href( "http://www.gnome.org", "GNOME Web Site" )@}
//!@xml{<image>../images/gnome2_href.png</image>@}
//!
//!@expr{ GTK2.Gnome2Href( "http://www.gnome.org" )@}
//!@xml{<image>../images/gnome2_href_2.png</image>@}
//!
//! 
//! Properties:
//! string text
//! string url
//! 
//! Style properties:
//! GDK.Color link-color
//!
//!

inherit GTK2.Button;

static Gnome2.Href create( string url, string|void label );
//! Created a GNOME href object, a label widget with a clickable action
//! and an associated URL. If label is set to 0, url is used as the
//! label.
//!
//!

string get_text( );
//! Returns the contents of the label widget used to display the link text.
//!
//!

string get_url( );
//! Return the url
//!
//!

Gnome2.Href set_text( string text );
//! Sets the internal label widget text (used to display a URL's link
//! text) to the given value.
//!
//!

Gnome2.Href set_url( string url );
//! Sets the internal URL
//!
//!
