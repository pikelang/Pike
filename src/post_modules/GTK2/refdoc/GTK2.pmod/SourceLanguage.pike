//!

inherit G.Object;

int get_escape_char( );
//! Gets the value of the ESC character in the given language.
//!
//!

array get_mime_types( );
//! Returns a list of mime types for this language.
//!
//!

string get_name( );
//! Returns the localized name of the language.
//!
//!

string get_section( );
//! Returns the localized section of the language.  Each language belongs to a
//! section (ex. HTML belongs to the Markup section).
//!
//!

GTK2.SourceStyleScheme get_style_scheme( );
//! Gets the style scheme associated with this language.
//!
//!

mapping get_tag_default_style( string id );
//! Gets the default style of the tag id.
//!
//!

mapping get_tag_style( string id );
//! Gets the style of the tag id.  If the style is not defined, then returns
//! the default style.
//!
//!

array get_tags( );
//! Returns a list of tags for this language.
//!
//!

GTK2.SourceLanguage set_mime_types( array|void types );
//! Sets a list of mime types for this language.  If omitted, this function
//! will use the default mime types from the language file.
//!
//!

GTK2.SourceLanguage set_tag_style( string id, mapping|void style );
//! Sets the style of the tag id.  If style is omitted, this function will
//! restore the default style.
//!
//!
