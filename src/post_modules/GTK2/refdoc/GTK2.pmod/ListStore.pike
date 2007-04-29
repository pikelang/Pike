//! A list-like data structure that can be used with W(TreeView).
//!
//!

inherit G.Object;

inherit GTK2.TreeModel;

inherit GTK2.TreeSortable;

inherit GTK2.TreeDragSource;

inherit GTK2.TreeDragDest;

GTK2.TreeIter append( );
//! Append a new row.
//!
//!

GTK2.ListStore clear( );
//! Removes all rows.
//!
//!

static GTK2.ListStore create( array types );
//! Create a new list store with as many columns as there are items in the
//! array. A type is either a string representing a type name, such as "int" or
//! "float", or an actual widget.  If it is a widget, the column in question
//! should be a widget of the same type that you would like this column to
//! represent.
//!
//!

GTK2.TreeIter insert( int position );
//! Insert a row at position.  If parent is valid, will create as child,
//! otherwise at top level.  If position is larger than then number
//! of rows at that level, it will be added to the end of the list.
//! iter will be changed to point to the new row.
//!
//!

GTK2.TreeIter insert_after( GTK2.TreeIter sibling );
//! Insert a new row after sibling.  If sibling is 0, then the row
//! will be prepended to parent's children.  If parent and sibling
//! are both 0, then the row will be prepended to the toplevel.
//!
//!

GTK2.TreeIter insert_before( GTK2.TreeIter sibling );
//! Insert a row before sibling.  If sibling is 0, then the
//! row will be appended to parent's children.  If parent and
//! sibling are 0, then the row will be appended to the toplevel.
//!
//!

GTK2.ListStore move_after( GTK2.TreeIter iter, GTK2.TreeIter position );
//! Moves iter to after position.  These should be at the
//! same level.  This only works if the store is unsorted.
//! If position is omitted, iter will be moved to the start
//! of the level.
//!
//!

GTK2.ListStore move_before( GTK2.TreeIter iter, GTK2.TreeIter position );
//! Moves iter to before position.  These should be at the
//! same level.  This only works if the store is unsorted.
//! If position is omitted, iter will be moved to the end
//! of the level.
//!
//!

GTK2.TreeIter prepend( );
//! Prepend a new row. 
//!
//!

GTK2.ListStore remove( GTK2.TreeIter iter );
//! Remove iter.
//! iter is set to the next valid row at that level,
//! or invalidated if it was the last one.
//!
//!

GTK2.ListStore set_row( GTK2.TreeIter iter, array values );
//! Set the data in an entire row.
//!
//!

GTK2.ListStore set_value( GTK2.TreeIter iter, int column, mixed value );
//! Set the data in the cell specified by iter and column.
//!
//!

GTK2.ListStore swap( GTK2.TreeIter a, GTK2.TreeIter b );
//! Swap 2 rows.  Only works if this store is unsorted.
//!
//!
