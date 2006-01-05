//!

inherit G.Object;

static GTK2.SourceLanguagesManager create( );
//! Create a new language manager.
//!
//!

array get_available_languages( );
//! Gets a list of available languages.
//!
//!

array get_lang_files_dirs( );
//! Gets a list of language files directories.
//!
//!

GTK2.SourceLanguage get_language_from_mime_type( string type );
//! Gets the GTK2.SourceLanguage which is associated with the given type in the
//! language manager.
//!
//!

GTK2.SourceLanguage get_language_from_name( string name );
//! Gets the GTK2.SourceLanguage which has this name.
//!
//!
