//! This is one of the nodes (branch or leaf) of the CTree.
//! They are the equivalent of row numbers in a normal CList.
//!
//!

GTK.CTreeNode child( );
//! Returns the first child node
//!
//!

int get_expanded( );
//! Returns the previous sibling (the next on the same level)
//!
//!

int get_is_leaf( );
//! Returns the previous sibling (the next on the same level)
//!
//!

int get_level( );
//! Returns the previous sibling (the next on the same level)
//!
//!

GTK.CTreeNode next( );
//! Returns the next sibling (the next on the same  level)
//!
//!

GTK.CTreeNode parent( );
//! Returns the parent node
//!
//!

object(implements 1001) prev( );
//! Returns the previous sibling (the next on the same level)
//!
//!

GTK.CTreeRow row( );
//! Returns the CTreeRow associated with this CTreeNode.
//! @b{DEPRECATED@}, all CTreeRow functions are also available
//! directly in this object.
//!
//!
