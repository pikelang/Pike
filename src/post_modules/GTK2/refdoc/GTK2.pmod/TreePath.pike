//! TreePath.
//!
//!


GTK2.TreePath append_index( int index );
//! Appends a new index to path.  As a result, the depth
//! of the path is increased.
//!
//!

int compare( GTK2.TreePath b );
//! Compares two paths.  If this path appears before b, -1 is returned.
//! If b before this path, 1 is return.  If they are equal, 0 is returned.
//!
//!

GTK2.TreePath copy( );
//! Creates a new GTK2.TreePath as a copy.
//!
//!

static GTK2.TreePath create( string|void path );
//! Creates a new GTK2.TreePath.
//!
//!

GTK2.TreePath destroy( );
//! Destructor.
//!
//!

GTK2.TreePath down( );
//! Moves path to point to the first child of the current path.
//!
//!

int get_depth( );
//! Returns the current depth of path.
//!
//!

array get_indices( );
//! Returns the current indices of path as an array
//! of integers, each representing a node in a tree.
//!
//!

int is_ancestor( GTK2.TreePath descendant );
//! Returns TRUE if descendant is a descendant of this path.
//!
//!

int is_descendant( GTK2.TreePath ancestor );
//! Returns TRUE if this path is a descendant of ancestor.
//!
//!

GTK2.TreePath next( );
//! Moves the path to point to the next node at the current depth.
//!
//!

GTK2.TreePath prepend_index( int index );
//! Prepends a new index to a path.  As a result, the depth
//! of the path is increased.
//!
//!

GTK2.TreePath prev( );
//! Moves the path to point to the previous node at the current depth,
//! if it exists.  Returns TRUE if the move was made.
//!
//!

string to_string( );
//! Generates a string representation of the path.
//! This string is a ':' separated list of numbers.
//! For example, "4:10:0:3" would be an acceptable return value
//! for this string.
//!
//!

GTK2.TreePath up( );
//! Moves the path to point to its parent node, if it has a parent.
//!
//!
