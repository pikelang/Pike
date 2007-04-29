//! Properties:
//! GTK2.TreeModel model
//!
//!

inherit G.Object;

inherit GTK2.TreeModel;

inherit GTK2.TreeDragSource;

inherit GTK2.TreeSortable;

GTK2.TreeIter convert_child_iter_to_iter( GTK2.TreeIter child_iter );
//! Returns an iter pointing to the row in this model that corresponds
//! to the row pointed at by child_iter.
//!
//!

GTK2.TreePath convert_child_path_to_path( GTK2.TreePath child_path );
//! Converts child_path to a path relative to this model.  That is,
//! child_path points to a path in the child mode.  The returned path will
//! point to the same row in the sorted model.
//!
//!

GTK2.TreeIter convert_iter_to_child_iter( GTK2.TreeIter sorted_iter );
//! Returns an iter pointing to the row in this model that corresponds
//! to the row pointed at by sorted_iter.
//!
//!

GTK2.TreePath convert_path_to_child_path( GTK2.TreePath sorted_path );
//! Converts sorted_path to a path on the child model.
//!
//!

static GTK2.TreeModelSort create( GTK2.TreeModel model );
//! Create a new GTK2.TreeModel, with model as the child model.
//!
//!

GTK2.TreeModel get_model( );
//! Return the model this ModelSort is sorting.
//!
//!

GTK2.TreeModelSort reset_default_sort_func( );
//! This resets the default sort function to be in the 'unsorted' state.
//! That is, it is in the same order as the child model.  It will re-sort the
//! model to be in the same order as the child model only if this model
//! is in 'unsorted' state.
//!
//!
