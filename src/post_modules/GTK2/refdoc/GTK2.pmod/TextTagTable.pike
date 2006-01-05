//! Table of text tags.
//!
//!
//!  Signals:
//! @b{tag_added@}
//!
//! @b{tag_changed@}
//!
//! @b{tag_removed@}
//!

inherit G.Object;

GTK2.TextTagTable add( GTK2.TextTag tag );
//! Add a tag to the table.  The tag is assigned the highest priority
//! in the table.
//!
//!

static GTK2.TextTagTable create( );
//! Creates a new W(TextTagTable).
//!
//!

int get_size( );
//! Returns the size of the table (number of tags).
//!
//!

GTK2.TextTag lookup( string name );
//! Look up a named tag.
//!
//!

GTK2.TextTagTable remove( GTK2.TextTag tag );
//! Remove a tag from the table.
//!
//!
