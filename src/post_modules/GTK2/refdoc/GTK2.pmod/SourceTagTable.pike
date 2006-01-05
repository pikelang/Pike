//! Derived from GTK2.TextTagTable
//!
//!
//!  Signals:
//! @b{changed@}
//!

inherit GTK2.TextTagTable;

GTK2.SourceTagTable add_tags( array tags );
//! Adds a list of tags to the table.  The added tags are assigned the highest
//! priority in the table.
//! 
//! If a tag is already present in the table or has the same name as an already
//! added tag, then it is not added to the table.
//!
//!

static GTK2.SourceTagTable create( );
//! Creates a new table.  The table contains no tags be default.
//!
//!

GTK2.SourceTagTable remove_source_tags( );
//! Removes all source tags from the table.  This will remove the table's
//! reference to the tags, so be careful - tags will end up destroyed if you
//! don't have a reference to them.
//!
//!
