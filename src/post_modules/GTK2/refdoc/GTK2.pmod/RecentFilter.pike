//! A filter for selecting a subset of recently used files.
//!
//!

inherit GTK2.Object;

GTK2.RecentFilter add_age( int days );
//! Adds a rule that allows resources based on their age - that is, the
//! number of days elapsed since they were last modified.
//!
//!

GTK2.RecentFilter add_application( string application );
//! Adds a rule that allows resources based on the name of the application
//! that has registered them.
//!
//!

GTK2.RecentFilter add_group( string group );
//! Adds a rule that allows resources based on the name of the group to
//! which they belong.
//!
//!

GTK2.RecentFilter add_mime_type( string mime_type );
//! Adds a rule that allows resources based on their registered MIME type.
//!
//!

GTK2.RecentFilter add_pattern( string pattern );
//! Adds a rule that allows resources based on a pattern matching their
//! display name.
//!
//!

GTK2.RecentFilter add_pixbuf_formats( );
//! Adds a rule allowing image files in the formats supported by GdkPixbuf.
//!
//!

protected GTK2.RecentFilter create( mapping|void props );
//! Create a new GTK2.RecentFilter.
//!
//!

string get_name( );
//! Gets the human-readable name for the filter.
//!
//!

GTK2.RecentFilter set_name( string name );
//! Sets the human-readable name of the filter; this is the string that
//! will be displayed in the recently used resources selector user
//! interface if there is a selectable list of filters.
//!
//!
