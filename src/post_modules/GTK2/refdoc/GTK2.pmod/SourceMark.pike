//!
//!
//!

inherit GTK2.TextMark;

string get_category( );
//! Gets the category type of this marker.
//!
//!

GTK2.SourceMark next( string category );
//! Gets the next marker of the specified category after this.
//!
//!

GTK2.SourceMark prev( string category );
//! Gets the previous marker of the specified category before this.
//!
//!
