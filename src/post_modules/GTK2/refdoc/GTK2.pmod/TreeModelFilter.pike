//! Properties:
//! GTK2.TreeModel child-model
//! GTK2.TreePath virtual-root
//!
//!

inherit G.Object;

inherit GTK2.TreeModel;

inherit GTK2.TreeDragSource;

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

static GTK2.TreeModelFilter create( GTK2.TreeModel model, GTK2.TreePath root );
//! Create a new GTK2.TreeModel, with model as the child model.
//!
//!

GTK2.TreeModelFilter refilter( );
//! Emits "row-changed" for each row in the child model, which causes the
//! filter to re-evaluate whether a row is visible or not.
//!
//!

GTK2.TreeModelFilter set_visible_func( function f, mixed user_data );
//! Sets the comparison function used when sorting to be f.  If the current
//! sort column id of this sortable is the same as column, then the model will
//! sort using this function.
//!
//!
