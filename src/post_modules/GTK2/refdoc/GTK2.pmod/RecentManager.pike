//!

inherit G.Object;

int add_item( string uri );
//! Adds a new resource, pointed by uri, into the recently used
//! resources list.
//!
//!

protected GTK2.RecentManager create( mapping|int|GdkScreen props_or_def );
//! Create a new GTK2.RecentManager.
//!
//!

array get_items( );
//! Gets the list of recently used resources.
//!
//!

int get_limit( );
//! Gets the maximum number of items that the get_items() function
//! should return.
//!
//!

int has_item( string uri );
//! Checks whether there is a recently used resource registered with uri
//! inside the recent manager.
//!
//!

mapping lookup_item( string uri );
//! Searches for a URI inside the recently used resources list, and returns
//! a structure containing information about the resource like its MIME type,
//! or its display name.
//!
//!

int move_item( string uri, string new_uri );
//! Changes the location of a recently used resource from uri to new_uri.
//!
//!

int purge_items( );
//! Purges every item from the recently used resources list.
//!
//!

int remove_item( string uri );
//! Removes a resource pointed by uri from the recently used resources list
//! handled by a recent manager.
//!
//!

GTK2.RecentManager set_limit( int limit );
//! Sets the maximum number of item that the get_items() function should
//! return. If limit is set to -1, then return all the items.
//!
//!

GTK2.RecentManager set_screen( GTK2.GdkScreen screen );
//! Sets the screen for a recent manager; the screen is used to track the
//! user's currently configured recently used documents storage.
//!
//!
