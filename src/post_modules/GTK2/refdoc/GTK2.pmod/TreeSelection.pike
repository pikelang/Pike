// Automatically generated from "gtktreeselection.pre".
// Do NOT edit.

//! Selections on a treestore.
//!
//!
//!  Signals:
//! @b{changed@}
//!

inherit G.Object;
//!

int count_selected_rows( );
//! Returns the number of rows that have been selected.
//!
//!

int get_mode( );
//! Gets the selectiom mode.
//!
//!

array get_selected( );
//! Returns a W(TreeITer) with the currently selected node if this selection
//! is set to GTK2.SELECTION_SINGLE or GTK2.SELECTION_BROWSE.  Also returns
//! W(TreeModel) as a convenience.  This function will not work if you this
//! selection is set to GTK2.SELECTION_MULTIPLE.
//!
//!

array get_selected_rows( GTK2.TreeModel model );
//! Creates a list of W(TreePath)'s for all selected rows.  Additionally, if
//! you are planning on modified the model after calling this function, you
//! may want to convert the returned list into a list of W(TreeRowReference)s.
//! To do this, you can use GTK2.TreeRowReference->create().
//!
//!

GTK2.TreeView get_tree_view( );
//! Returns the tree view associated with this selection.
//!
//!

object get_user_data( );
//! Returns the user data for the selection function.
//!
//!

int iter_is_selected( GTK2.TreeIter iter );
//! Returns true if the row at iter is currently selected.
//!
//!

int path_is_selected( GTK2.TreePath path );
//! Returns true if the row pointed to by path is currently selected.  If path
//! does not point to a valid location, false is returned.
//!
//!

GTK2.TreeSelection select_all( );
//! Selects all the nodes.  This selection must be set to 
//! GTK2.SELECTION_MULTIPLE mode.
//!
//!

GTK2.TreeSelection select_iter( GTK2.TreeIter iter );
//! Selects the specified iterator.
//!
//!

GTK2.TreeSelection select_path( GTK2.TreePath path );
//! Select the row at path.
//!
//!

GTK2.TreeSelection select_range( GTK2.TreePath start, GTK2.TreePath end );
//! Selects a range of nodes, determined by start and end inclusive.
//! This selection must be set to GTK2.SELECTION_MULTIPLE mode.
//!
//!

GTK2.TreeSelection set_mode( int type );
//! Sets the selection mode.  If the previous type was GTK2.SELECTION_MULTIPLE,
//! then the anchor is kept selected, if it was previously selected.
//! One of @[SELECTION_BROWSE], @[SELECTION_MULTIPLE], @[SELECTION_NONE] and @[SELECTION_SINGLE].
//!
//!

GTK2.TreeSelection unselect_all( );
//! Unselects all the nodes.
//!
//!

GTK2.TreeSelection unselect_iter( GTK2.TreeIter iter );
//! Unselects the specified iterator.
//!
//!

GTK2.TreeSelection unselect_path( GTK2.TreePath path );
//! Unselects the row at path.
//!
//!

GTK2.TreeSelection unselect_range( GTK2.TreePath start, GTK2.TreePath end );
//! Unselects a range of nodes, determined by start and end inclusive.
//!
//!
