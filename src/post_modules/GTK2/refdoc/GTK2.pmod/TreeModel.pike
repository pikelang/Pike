//! Class used for the TreeModel funtions.
//!
//!
//!  Signals:
//! @b{row_changed@}
//!
//! @b{row_deleted@}
//!
//! @b{row_has_child_toggled@}
//!
//! @b{row_inserted@}
//!
//! @b{rows_reordered@}
//!

inherit G.Object;

int get_column_type( int index );
//! Returns the type of the column.
//!
//!

int get_flags( );
//! Returns a set of flags supported by this interface.
//!
//!

GTK2.TreeIter get_iter( GTK2.TreePath path );
//! Returns a valid iterator pointer to path
//!
//!

GTK2.TreeIter get_iter_first( );
//! Get GTK2.TreeIter with the first iterator in the tree ("0").
//!
//!

GTK2.TreeIter get_iter_from_string( string path );
//! Returns a valid iterator from a path string.
//!
//!

int get_n_columns( );
//! Returns the number of columns suppported by this model.
//!
//!

GTK2.TreePath get_path( GTK2.TreeIter iter );
//! Returns a GTK2.TreePath from iter.
//!
//!

array get_row( GTK2.TreeIter iter );
//! Get the row at this iter.
//!
//!

string get_string_from_iter( GTK2.TreeIter iter );
//! Get string representation of iter.
//!
//!

mixed get_value( GTK2.TreeIter iter, int column );
//! Get value at column of iter.
//!
//!

GTK2.TreeIter iter_children( GTK2.TreeIter parent );
//! Get first child of parent.
//!
//!

int iter_has_child( GTK2.TreeIter iter );
//! Check if iter has children.
//!
//!

int iter_n_children( GTK2.TreeIter iter );
//! Returns the number of children this iter has.
//!
//!

GTK2.TreeIter iter_next( GTK2.TreeIter iter );
//! Go to next node.
//!
//!

GTK2.TreeIter iter_nth_child( GTK2.TreeIter parent, int index );
//! Get the child of parent using the given index.
//! Returns valid GTK2.TreeIter if it exists, or 0.
//! If the index is too big, or parent is invalid,
//! then it returns the index from the root node.
//!
//!

GTK2.TreeIter iter_parent( GTK2.TreeIter child );
//! Get parent of child, or 0 if none.
//!
//!

GTK2.TreeModel row_changed( GTK2.TreePath path, GTK2.TreeIter iter );
//! Emit "row-changed" signal.
//!
//!

GTK2.TreeModel row_deleted( GTK2.TreePath path );
//! Emit "row-deleted" signal.
//!
//!

GTK2.TreeModel row_has_child_toggled( GTK2.TreePath path, GTK2.TreeIter iter );
//! Emit "row-has-child-toggled" signal.
//!
//!

GTK2.TreeModel row_inserted( GTK2.TreePath path, GTK2.TreeIter iter );
//! Emit "row-inserted" signal.
//!
//!
