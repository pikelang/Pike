//!

inherit GTK2.Data;

GTK2.FileFilter add_mime_type( string mime_type );
//! Adds a rule allowing a given mime type.
//!
//!

GTK2.FileFilter add_pattern( string pattern );
//! Adds a rule allowing a shell style glob.
//!
//!

GTK2.FileFilter add_pixbuf_formats( );
//! Adds a rule allowing image files in the formats support by W(Pixbuf).
//!
//!

static GTK2.FileFilter create( );
//! Creates a new W(FileFilter) with no rules added to it.  Such a filter
//! doesn't accept any files, so is not particularly useful until you add rules
//! with add_mime_type(), add_pattern(), or add_custom().  To create a filter
//! accepts any file, use add_pattern("*").
//!
//!

string get_name( );
//! Gets the human-readable name for the filter.
//!
//!

GTK2.FileFilter set_name( string name );
//! Sets the human-readable name of the filter; this is the string that will
//! be displayed in the file selector user interface if there is a selectable
//! list of filters.
//!
//!
