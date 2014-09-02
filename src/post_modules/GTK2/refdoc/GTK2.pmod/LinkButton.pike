//! Properties:
//! 
//! string uri
//! A GTK2.LinkButton is a GTK2.Button with a hyperlink, similar to the one
//! used by web browsers, which triggers an action when clicked. It is useful
//! to show quick links to resources.
//!
//!

inherit GTK2.Button;

protected GTK2.LinkButton create( string|mapping uri_or_props, string|void label );
//! Creates a new LinkButton.
//!
//!

string get_uri( );
//! Retrieves the URI set using set_uri().
//!
//!

GTK2.LinkButton set_uri( string uri );
//! Sets uri as the URI.
//!
//!
