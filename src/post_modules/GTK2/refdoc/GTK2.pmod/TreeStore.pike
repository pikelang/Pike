//! A tree-like data structure that can be used with W(TreeView).
//!
//!
//!  Signals:
//! @b{sort_column_changed@}
//!

inherit GTK2.TreeModel;

GTK2.TreeIter append( GTK2.TreeIter parent );
//! Append a new row.  If parent is valid, then it will append
//! the new row after the last child, otherwise it will append
//! a row to the toplevel.
//!
//!

GTK2.TreeStore clear( );
//! Removes all rows.
//!
//!

static GTK2.TreeStore create( array types );
//! Create a new tree store with as many columns as there are items in the
//! array. A type is either a string representing a type name, such as "int" or
//! "float", or an actual widget.  If it is a widget, the column in question
//! should be a widget of the same type that you would like this column to
//! represent.
//!
//!

int drag_data_delete( GTK2.TreePath path );
//! Asks to delete the row at path, because it was moved somewhere else via
//! drag-and-drop.  Returns false if the deletion fails because path no longer
//! exists, or for some model-specific reason.
//!
//!

GTK2.SelectionData drag_data_get( GTK2.TreePath path );
//! Asks to return a representation of the row at path.
//!
//!

int drag_data_received( GTK2.TreePath path, GTK2.SelectionData sel );
//! Asks to insert a row before the path dest, deriving the contents of the
//! row from the sel.  If this dest is outside the tree so that inserting
//! before it is impossible, false will be returned.  Also, false may be
//! returned if the new row is not created for some model-specific reason.
//!
//!

mapping get_sort_column_id( );
//! Returns ([ "column": int, "order": int ])
//!
//!

int has_default_sort_func( );
//! Returns true if the model has a default sort function.
//!
//!

GTK2.TreeIter insert( GTK2.TreeIter parent, int position );
//! Insert a row at position.  If parent is valid, will create as child,
//! otherwise at top level.  If position is larger than then number
//! of rows at that level, it will be added to the end of the list.
//! iter will be changed to point to the new row.
//!
//!

GTK2.TreeIter insert_after( GTK2.TreeIter parent, GTK2.TreeIter sibling );
//! Insert a new row after sibling.  If sibling is 0, then the row
//! will be prepended to parent's children.  If parent and sibling
//! are both 0, then the row will be prepended to the toplevel.
//!
//!

GTK2.TreeIter insert_before( GTK2.TreeIter parent, GTK2.TreeIter sibling );
//! Insert a row before sibling.  If sibling is 0, then the
//! row will be appended to parent's children.  If parent and
//! sibling are 0, then the row will be appended to the toplevel.
//!
//!

int is_ancestor( GTK2.TreeIter iter, GTK2.TreeIter descendant );
//!  Returns true if iter is an ancestor of descendant.
//!
//!

int iter_depth( GTK2.TreeIter iter );
//! Get the depth of iter.  This will be 0 for anything
//! on the root level, 1 for anything down a level, and
//! so on.
//!
//!

GTK2.TreeIter prepend( GTK2.TreeIter parent );
//! Prepend a new row. If parent is valid, then it will prepend
//! the new row before the first child of parent, otherwise it will
//! prepend a new row to the toplevel.
//!
//!

GTK2.TreeStore remove( GTK2.TreeIter iter );
//! Remove iter.
//! iter is set to the next valid row at that level,
//! or invalidated if it was the last one.
//!
//!

int row_draggable( GTK2.TreePath path );
//! Asks the source whether a particular row can be used as the source of a
//! DND operation.  If the source doesn't implement this interface, the row
//! is assumed draggable.
//!
//!

int row_drop_possible( GTK2.TreePath path, GTK2.SelectionData sel );
//! Determines whether a drop is possible before past, at the same depth as
//! path.  i.e., can we drop the data in sel at that location.  path does not
//! have to exist; the return value will almost certainly be false if the
//! parent of path doesn't exist, though.
//!
//!

GTK2.TreeStore set_default_sort_func( function f, mixed user_data );
//! Sets the default comparison function used when sorting to be f.  If the
//! current sort column id of this sortable is 
//! GTK2.TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, then the model will sort using
//! this function.
//! 
//! if f is 0, then there will be no default comparison function.  This means
//! once the model has been sorted, it can't go back to the default state.  In
//! this case, when the current sort column id of this sortable is
//! GTK2.TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, the model will be unsorted.
//!
//!

GTK2.TreeStore set_row( GTK2.TreeIter iter, array values );
//! Set the data in an entire row.
//!
//!

GTK2.TreeStore set_sort_column_id( int column, int order );
//! Sets the current sort column to be column.  The widget will resort itself
//! to reflect this change, after emitting a "sort-column-changed" signal.
//! If column is GTK2.TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, then the default
//! sort function will be used, if it is set.
//!
//!

GTK2.TreeStore set_sort_func( int column, function f, mixed user_data );
//! Sets the comparison function used when sorting to be f.  If the current
//! sort column id of this sortable is the same as column, then the model will
//! sort using this function.
//!
//!

GTK2.TreeStore set_value( GTK2.TreeIter iter, int column, mixed value );
//! Set the data in the cell specified by iter and column.
//!
//!

GTK2.TreeStore sort_column_changed( );
//! Emits a "sort-column-changed" signal.
//!
//!
