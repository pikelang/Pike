//!

GTK2.RecentChooser add_filter( GTK2.RecentFilter filter );
//! Adds filter to the list of GTK2.RecentFilter objects held by chooser.
//!
//!

GTK2.RecentInfo get_current_item( );
//! Gets the GTK2.RecentInfo currently selected.
//!
//!

string get_current_uri( );
//! Gets the URI currently selected.
//!
//!

GTK2.RecentFilter get_filter( );
//! Gets the filter currently used.
//!
//!

array get_items( );
//! Gets the list of recently used resources in form of GTK2.RecentInfo
//! objects.
//! 
//! The return value of this function is affected by the "sort-type" and
//! "limit" properties of chooser
//!
//!

int get_limit( );
//! Gets the number of items returned by get_items() and get_uris().
//!
//!

int get_local_only( );
//! Gets whether only local resources should be shown in the recently used
//! resources selector.
//!
//!

int get_select_multiple( );
//! Gets whether chooser can select multiple items.
//!
//!

int get_show_icons( );
//! Retrieves whether chooser should show an icon near the resource.
//!
//!

int get_show_not_found( );
//! Retrieves whether chooser should show the recently used resources that
//! were not found.
//!
//!

int get_show_numbers( );
//! Returns whether chooser should display recently used resources prepended
//! by a unique number.
//!
//!

int get_show_private( );
//! Returns whether chooser should display recently used resources registered
//! as private.
//!
//!

int get_show_tips( );
//! Gets whether chooser should display tooltips.
//!
//!

int get_sort_type( );
//! Gets the sorting order.
//!
//!

array get_uris( );
//! Gets the URI of the recently used resources.
//! 
//! The return value of this function is affected by the "sort-type"
//! and "limit" properties.
//!
//!

array list_filters( );
//! Gets a list of filters.
//!
//!

GTK2.RecentChooser remove_filter( GTK2.RecentFilter filter );
//! Removes filter.
//!
//!

GTK2.RecentChooser select_all( );
//! Selects all the items inside chooser, if the chooser supports
//! multiple selection.
//!
//!

int select_uri( string uri );
//! Selects uri.
//!
//!

int set_current_uri( string uri );
//! Sets uri as the current URI.
//!
//!

GTK2.RecentChooser set_filter( GTK2.RecentFilter filter );
//! Sets filter as the current GTK2.RecentFilter object to affect the
//! displayed recently used resources.
//!
//!

GTK2.RecentChooser set_limit( int limit );
//! Sets the number of items that should be returned by get_items() and
//! get_uris().
//!
//!

GTK2.RecentChooser set_local_only( int local_only );
//! Sets whether only local resources, that is resources using the
//! file:// URI scheme, should be shown in the recently used resources
//! selector. If local_only is TRUE (the default) then the shown resources
//! are guaranteed to be accessible through the operating system native
//! file system.
//!
//!

GTK2.RecentChooser set_select_multiple( int select_multiple );
//! Sets whether chooser can select multiple items.
//!
//!

GTK2.RecentChooser set_show_icons( int show_icons );
//! Sets whether chooser should show an icon near the resource when
//! displaying it.
//!
//!

GTK2.RecentChooser set_show_not_found( int show_not_found );
//! Sets whether chooser should display the recently used resources that
//! it didn't find. This only applies to local resources.
//!
//!

GTK2.RecentChooser set_show_numbers( int show_numbers );
//! Whether to show recently used resources prepended by a unique number.
//!
//!

GTK2.RecentChooser set_show_private( int show_private );
//! Whether to show recently used resources marked registered as private.
//!
//!

GTK2.RecentChooser set_show_tips( int show_tips );
//! Sets whether to show a tooltips on the widget.
//!
//!

GTK2.RecentChooser set_sort_type( int sort_type );
//! Changes the sorting order of the recently used resources list displayed
//! by chooser.
//!
//!

GTK2.RecentChooser unselect_all( );
//! Unselects all the items.
//!
//!

GTK2.RecentChooser unselect_uri( string uri );
//! Unselects uri.
//!
//!
