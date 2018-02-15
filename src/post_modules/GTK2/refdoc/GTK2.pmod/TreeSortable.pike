//! The interface for sortable models used by TreeView.
//!
//!
//!  Signals:
//! @b{sort_column_changed@}
//!

mapping get_sort_column_id( );
//! Returns ([ "column": int, "order": int ])
//!
//!

int has_default_sort_func( );
//! Returns true if the model has a default sort function.
//!
//!

GTK2.TreeSortable set_default_sort_func( function f, mixed user_data );
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

GTK2.TreeSortable set_sort_column_id( int column, int order );
//! Sets the current sort column to be column.  The widget will resort itself
//! to reflect this change, after emitting a "sort-column-changed" signal.
//! If column is GTK2.TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, then the default
//! sort function will be used, if it is set.
//!
//!

GTK2.TreeSortable set_sort_func( int column, function f, mixed user_data );
//! Sets the comparison function used when sorting to be f.  If the current
//! sort column id of this sortable is the same as column, then the model will
//! sort using this function.
//!
//!

GTK2.TreeSortable sort_column_changed( );
//! Emits a "sort-column-changed" signal
//!
//!
