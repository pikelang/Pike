//!

GTK2.RecentInfo destroy( );
//! Destructor.
//!
//!

int exists( );
//! Checks whether the resource pointed by info still exists. At the moment
//! this check is done only on resources pointing to local files.
//!
//!

int get_added( );
//! Gets the timestamp (seconds from system's Epoch) when the resource was
//! added to the recently used resources list.
//!
//!

int get_age( );
//! Gets the number of days elapsed since the last update of the resource
//! pointed by info.
//!
//!

string get_description( );
//! Gets the (short) description of the resource.
//! Gets the (short) description of the resource.
//!
//!

string get_display_name( );
//! Gets the name of the resource. If none has been defined, the basename
//! of the resource is obtained.
//!
//!

array get_groups( );
//! Returns all groups registered for the recently used item.
//!
//!

GTK2.GdkPixbuf get_icon( int size );
//! Retrieves the icon of size size associated to the resource MIME type.
//!
//!

string get_mime_type( );
//! Gets the MIME type of the resource.
//!
//!

int get_modified( );
//! Gets the timestamp (seconds from system's Epoch) when the resource was
//! last modified.
//!
//!

int get_private_hint( );
//! Gets the value of the "private" flag. Resources in the recently used
//! list that have this flag set to TRUE should only be displayed by the
//! applications that have registered them.
//!
//!

string get_short_name( );
//! Computes a string that can be used as the name of the item
//! in a menu or list. For example, calling this function on an item that
//! refers to "file:///foo/bar.txt" will yield "bar.txt".
//!
//!

string get_uri( );
//! Gets the URI of the resource.
//!
//!

string get_uri_display( );
//! Gets a displayable version of the resource's URI.
//!
//!

int get_visited( );
//! Gets the timestamp (seconds from system's Epoch) when the resource was
//! last visited.
//!
//!

int has_application( );
//! Checks whether an application registered this resource using app_name.
//!
//!

int has_group( string group_name );
//! Checks whether group_name appears inside the groups registered for the
//! recently used item info.
//!
//!

int is_local( );
//! Checks whether the resource is local or not by looking at the scheme
//! of its URI.
//!
//!

string last_application( );
//! Gets the name of the last application that have registered the recently
//! used resource represented by info.
//!
//!

int match( GTK2.RecentInfo b );
//! Checks whether two GTK2.RecentInfo structures point to the same resource.
//!
//!
