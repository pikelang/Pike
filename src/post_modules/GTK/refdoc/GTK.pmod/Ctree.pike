//! The GtkCTree widget is used for showing a hierarchical tree to the
//! user, for example a directory tree.
//! 
//! The tree is internally represented as a set of GtkCTreeNode
//! structures.
//! 
//! The interface has much in common with the GtkCList widget: rows
//! (nodes) can be selected by the user etc.
//! 
//! Positions in the tree are often indicated by two arguments, a
//! parent and a sibling, both GtkCTreeNode pointers. If the parent is
//! NULL, the position is at the root of the tree and if the sibling is
//! NULL, it will be the last child of parent, otherwise it will be
//! inserted just before the sibling.
//! 
//!
//!
//!  Signals:
//! @b{change_focus_row_expansion@}
//! Changed when the focused row is either expanded or collapsed
//!
//!
//! @b{tree_collapse@}
//! Called when a node is collapsed
//!
//!
//! @b{tree_expand@}
//! Called when a node is expanded
//!
//!
//! @b{tree_move@}
//! Called when a node is moved (using DND, as an example)
//!
//!
//! @b{tree_select_row@}
//! Called when a node is selected.
//!
//!
//! @b{tree_unselect_row@}
//! Called when a node is unselected.
//!
//!

inherit GTK.Clist;

GTK.Ctree collapse( GTK.CTreeNode node );
//! Collapse the node, hiding it's children.
//! If no node is given, expand the toplevel of the tree
//!
//!

GTK.Ctree collapse_recursive( GTK.CTreeNode node );
//! Collapse the node, showing it's children, it's childrens children, etc.
//! If no node is given, collapse the whole tree
//!
//!

GTK.Ctree collapse_to_depth( GTK.CTreeNode node, int depth );
//! Collapse depth levels of the tree, starting with the specified node.
//! If no node is given, start with the toplevel node.
//!
//!

static GTK.Ctree create( int columns, int tree_column );
//! tree_column is the column that has the tree graphics (lines and
//! expander buttons).
//!
//!

GTK.Ctree expand( GTK.CTreeNode node );
//! Expand the node, showing it's children.
//! If no node is given, expand the toplevel of the tree
//!
//!

GTK.Ctree expand_recursive( GTK.CTreeNode node );
//! Expand the node, showing it's children, it's childrens children, etc.
//! If no node is given, expand the whole tree
//!
//!

GTK.Ctree expand_to_depth( GTK.CTreeNode node, int depth );
//! Expand depth levels of the tree, starting with the specified node.
//! If no node is given, start with the toplevel node.
//!
//!

int find( GTK.CTreeNode node, GTK.CTreeNode start );
//! Returns true if the node is a child of the start node.
//! 
//! If you omit the starting node, the tree will be searched from
//! the root.
//!
//!

GTK.Ctree find_by_row_data( object data, CTreeNode root );
//! Find a node in the tree starting with root, that has the given user data.
//! If no node is found, 0 is returned.
//!
//!

GTK.CTreeNode find_node_ptr( GTK.CTreeRow node );
//! Given a W(CTreeRow) (deprectated structure in PiGTK), return the
//! W(CTreeNode) associated with the row.
//!
//!

int get_expander_style( );
//! The style of the expander buttons, one of @[CTREE_EXPANDER_CIRCULAR], @[CTREE_EXPANDER_NONE], @[CTREE_EXPANDER_SQUARE] and @[CTREE_EXPANDER_TRIANGLE]
//!
//!

int get_line_style( );
//! The style of the lines, one of @[CTREE_LINES_DOTTED], @[CTREE_LINES_NONE], @[CTREE_LINES_SOLID] and @[CTREE_LINES_TABBED]
//!
//!

int get_show_stub( );
//! Will stubs be shows?
//!
//!

int get_tree_column( );
//! The column that is the tree column (the one with the expand/collapse icons)
//!
//!

int get_tree_indent( );
//! The number of pixels to indent the tree levels.
//!
//!

int get_tree_spacing( );
//! The number of pixels between the tree and the columns
//!
//!

GTK.Ctree insert_node( GTK.CTreeNode parent, GTK.CTreeNode sibling, array text, int is_leaf, int expanded );
//! At least one of parent or sibling must be specified.
//! If both are specified, sibling->parent() must be equal to parent.
//! 
//! If the parent and sibling is 0, the position is at the root of the
//! tree, if the sibling is NULL, it will be the last child of parent,
//! otherwise it will be inserted just before the sibling.
//! 
//!
//!

int is_ancestor( GTK.CTreeNode node, GTK.CTreeNode child );
//! Returns true if @b{node@} is an ancestor of @b{child@}
//!
//!

int is_hot_spot( int x, int y );
//! Returns true if the given coordinates lie on an expander button
//!
//!

int is_viewable( GTK.CTreeNode node );
//! Returns 1 if a certain node can be viewed (with or without
//! scrolling of the tree).
//! Returns 0 if the node is in a folded part of the tree.
//!
//!

GTK.CTreeNode last( GTK.CTreeNode node );
//! Returns the last child of the last child of the last child... of
//! the given node.
//!
//!

GTK.Ctree move( GTK.CTreeNode node, GTK.CTreeNode new_parent, GTK.CTreeNode new_sibling );
//! Move a node. Coordinates work as for insert.
//!
//!

GTK.Style node_get_cell_style( GTK.CTreeNode node, int col );
//! Return the style of a cell
//!
//!

int node_get_cell_type( GTK.CTreeNode node, int column );
//! Return the celltype of this node.
//!
//!

mapping node_get_pixmap( CTreeNode node, int column );
//! Returns the pixmap and mask of this node in a mapping:
//! ([ "pixmap":the_pixmap, "mask":the_bitmap ])
//!
//!

mapping node_get_pixtext( GTK.CTreeNode n, int columne );
//! Returns the pixmap, mask and text of this node in a mapping:
//! ([ "pixmap":the_pixmap, "mask":the_bitmap, "text":the_text ])
//!
//!

object node_get_row_data( GTK.CTreeNode n );
//! Return the data associated with a node, or 0.
//!
//!

GTK.Style node_get_row_style( GTK.CTreeNode node );
//! Return the style of a row
//!
//!

int node_get_selectable( GTK.CTreeNode node );
//! Return whether or not this node can be selcted by the user
//!
//!

string node_get_text( GTK.CTreeNode node, int column );
//! Returns the text of the specified node
//!
//!

int node_is_visible( GTK.CTreeNode node );
//! Return 1 if the node is currently visible
//!
//!

GTK.Ctree node_moveto( GTK.CTreeNode row, int column, float row_align, float col_align );
//! Scroll the tree so a specified node (and column) is visible.
//! If the node is folded, it's first visible parent will be shown.
//!
//!

GTK.CTreeNode node_nth( int row );
//! Return the node that is currently visible on the specified row.
//!
//!

GTK.Ctree node_set_background( GTK.CTreeNode node, GDK.Color color );
//! Set the background of a row
//!
//!

GTK.Ctree node_set_cell_style( GTK.CTreeNode node, int col, GTK.Style style );
//! Set the style of a cell
//!
//!

GTK.Ctree node_set_foreground( GTK.CTreeNode node, GDK.Color col );
//! Set the foreground of a row
//!
//!

GTK.Ctree node_set_pixmap( GTK.CTreeNode node, int column, GDK.Pixmap pixmap, GDK.Bitmap mask );
//! Set the pixmap in a cell
//!
//!

GTK.Ctree node_set_pixtext( GTK.CTreeNode node, int column, string text, int spacing, GDK.Pixmap pixmap, GDK.Bitmap mask );
//! Set the pixmap and text in a cell
//!
//!

GTK.Ctree node_set_row_data( GTK.CTreeNode node, object data );
//! Set the user data associated with the specified node.
//! This data can be used to find nodes, and when a node is selected it
//! can be easily retrieved using node_get_row_data.
//! 
//! @b{You can only use objects as row data right now@}
//!
//!

GTK.Ctree node_set_row_style( GTK.CTreeNode node, GTK.Style style );
//! Set the style of a row
//!
//!

GTK.Ctree node_set_selectable( GTK.CTreeNode node, int selectablep );
//! Whether this node can be selected by the user.
//!
//!

GTK.Ctree node_set_shift( GTK.CTreeNode node, int column, int vertical, int horizontal );
//! Shift the given cell the given amounts in pixels.
//!
//!

GTK.Ctree node_set_text( GTK.CTreeNode node, int column, string text );
//! Set the text in a cell
//!
//!

GTK.Ctree remove_node( GTK.CTreeNode node );
//! Remove a node and it's subnodes from the tree.
//! The nodes will be destroyed, so you cannot add them again.
//!
//!

GTK.Ctree select( GTK.CTreeNode node );
//! Select a node.
//!
//!

GTK.Ctree select_recursive( GTK.CTreeNode node );
//! Select a node and it's children.
//!
//!

GTK.Ctree set_expander_style( int style );
//! Set the expander style, one of @[CTREE_EXPANDER_CIRCULAR], @[CTREE_EXPANDER_NONE], @[CTREE_EXPANDER_SQUARE] and @[CTREE_EXPANDER_TRIANGLE]
//!
//!

GTK.Ctree set_indent( int npixels );
//! Set the indentation level
//!
//!

GTK.Ctree set_line_style( int style );
//! Set the line style, one of @[CTREE_LINES_DOTTED], @[CTREE_LINES_NONE], @[CTREE_LINES_SOLID] and @[CTREE_LINES_TABBED]
//!
//!

GTK.Ctree set_node_info( GTK.CTreeNode node, string text, int spacing, GDK.Pixmap pixmap_closed, GDK.Bitmap mask_closed, GDK.Pixmap pixmap_opened, GDK.Bitmap mask_opened, int is_leaf, int expanded );
//! @xml{<matrix>@}
//! @xml{<r>@}@xml{<c>@} text :@xml{</c>@}@xml{<c>@}The texts to be shown in each column.@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@}spacing :@xml{</c>@}
//! @xml{<c>@}The extra space between the pixmap and the text.@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@}pixmap_closed :@xml{</c>@}
//! @xml{<c>@}The pixmap to be used when the node is collapsed. Can be NULL.@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@} mask_closed :@xml{</c>@}
//! @xml{<c>@}The mask for the above pixmap. Can be NULL.@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@} pixmap_opened :@xml{</c>@}
//! @xml{<c>@}The pixmap to be used when the children are visible. Can be NULL.@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@}  mask_opened :@xml{</c>@}
//! @xml{<c>@}The mask for the above pixmap. Can be NULL.@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@}is_leaf :@xml{</c>@}
//! @xml{<c>@}Whether this node is going to be a leaf.@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@} expanded :@xml{</c>@}
//! @xml{<c>@}Whether this node should start out expanded or not.@xml{</c>@}@xml{</r>@}
//! @xml{</matrix>@}
//!
//!

GTK.Ctree set_show_stub( int stubp );
//! If true, the 'stub' will be shown. The stub is the small line that
//! goes horizontally from the expand or collapse button to the actual
//! contents of the tree
//!
//!

GTK.Ctree set_spacing( int npixels );
//! Set the spacing between the tree column and the other columns
//!
//!

GTK.Ctree sort_node( GTK.CTreeNode node );
//! Sort the specified node.
//!
//!

GTK.Ctree sort_recursive( GTK.CTreeNode node );
//! Sort the specified node and it's children.
//!
//!

GTK.Ctree toggle_expansion( GTK.CTreeNode node );
//! If the node is expanded, collapse it, and if it's collapsed, expand it.
//!
//!

GTK.Ctree toggle_expansion_recursive( GTK.CTreeNode node );
//! Toggle the expansion of the whole subtree, starting with node.
//!
//!

GTK.Ctree unselect( GTK.CTreeNode node );
//! Unselect a node.
//!
//!

GTK.Ctree unselect_recursive( GTK.CTreeNode node );
//! Unselect a node and it's children.
//!
//!
