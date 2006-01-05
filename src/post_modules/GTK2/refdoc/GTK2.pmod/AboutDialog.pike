//! Properties:
//! array(string) artists
//! array(string) authors
//! string comments
//! string copyright
//! array(string) documenters
//! string license
//! GDK2.Pixbuf logo
//! string logo-icon-name
//! string name
//! string translator-credits
//! string version
//! string website
//! string website-label
//! 
//! Style properties:
//! GDK2.Color link-color
//!
//!

inherit GTK2.Dialog;

static GTK2.AboutDialog create( mapping|void props );
//! Create a new GTK.AboutDialog.
//!
//!

array get_artists( );
//! Returns the strings which are displayed in the artists tab of the
//! secondary credits dialog.
//!
//!

array get_authors( );
//! Returns the strings which are displayed in the authors tab of the
//! secondary credits dialog.
//!
//!

string get_comments( );
//! Returns the comments string.
//!
//!

string get_copyright( );
//! Returns the copyright string.
//!
//!

array get_documenters( );
//! Returns the strings which are displayed in the documenters tab of the
//! secondary credits dialog.
//!
//!

string get_license( );
//! Returns the license information.
//!
//!

GTK2.GdkPixbuf get_logo( );
//! Returns the pixbuf displayed as logo.
//!
//!

string get_logo_icon_name( );
//! Returns the icon name.
//!
//!

string get_name( );
//! Returns the program name.
//!
//!

mixed get_property( string property );
//! Get property.
//!
//!

string get_translator_credits( );
//! Returns the translator credis.
//!
//!

string get_version( );
//! Returns the version string.
//!
//!

string get_website( );
//! Returns the website URL.
//!
//!

string get_website_label( );
//! Returns the label used for the website link.
//!
//!

int get_wrap_license( );
//! Returns whether the license text is automatically wrapped.
//!
//!

GTK2.AboutDialog set_artists( array art );
//! Sets the strings which are displayed in the artists tab of the secondary
//! credits dialog.
//!
//!

GTK2.AboutDialog set_authors( array auth );
//! Sets the strings which are displayed in the authors tab of the secondary
//! credits dialog.
//!
//!

GTK2.AboutDialog set_comments( string comments );
//! Sets the comment string.
//!
//!

GTK2.AboutDialog set_copyright( string copyright );
//! Sets the copyright string.
//!
//!

GTK2.AboutDialog set_documenters( array doc );
//! Sets the strings which are displayed in the documenters tab of the
//! secondary credits dialog.
//!
//!

GTK2.AboutDialog set_license( string license );
//! Sets the license information.
//!
//!

GTK2.AboutDialog set_logo( GTK2.GdkPixbuf logo );
//! Sets the pixbuf to be displayed as the logo.
//!
//!

GTK2.AboutDialog set_logo_icon_name( string name );
//! Sets the icon name.
//!
//!

GTK2.AboutDialog set_name( string name );
//! Sets the name to display.
//!
//!

GTK2.AboutDialog set_translator_credits( string credits );
//! Sets the translator credits.
//!
//!

GTK2.AboutDialog set_version( string version );
//! Sets the version string.
//!
//!

GTK2.AboutDialog set_website( string website );
//! Sets the URL to use for the website link.
//!
//!

GTK2.AboutDialog set_website_label( string label );
//! Sets the label used for the website link.  Defaults to the website URL.
//!
//!

GTK2.AboutDialog set_wrap_license( int setting );
//! Sets whether the license text is automatically wrapped.
//!
//!
