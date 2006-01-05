//! Properties:
//! int enable-search
//! GTK2.TreeViewColumn expander-column
//! int fixed-height-mode
//! GTK2.Adjustment hadjustment
//! int headers-clickable
//! int headers-visible
//! int hover-expand
//! int hover-selection
//! GTK2.TreeModel model
//! int reorderable
//! int rules-hint
//! int search-column
//! GTK2.Adjustment vadjustment
//! 
//! Style properties:
//! int allow-rules
//! GDK2.Color even-row-color
//! int expander-size
//! int horizontal-separator
//! int indent-expanders
//! GDK2.Color odd-row-color
//! int vertical-separator
//!
//!
//!  Signals:
//! @b{columns_changed@}
//!
//! @b{cursor_changed@}
//!
//! @b{expand_collapse_cursor_row@}
//!
//! @b{move_cursor@}
//!
//! @b{row_activated@}
//!
//! @b{row_collapsed@}
//!
//! @b{row_expanded@}
//!
//! @b{select_all@}
//!
//! @b{select_cursor_parent@}
//!
//! @b{select_cursor_row@}
//!
//! @b{set_scroll_adjustments@}
//!
//! @b{start_interactive_search@}
//!
//! @b{test_collapse_row@}
//!
//! @b{test_expand_row@}
//!
//! @b{toggle_cursor_row@}
//!
//! @b{unselect_all@}
//!

inherit GTK2.Container;

GTK2.TreeView append_column( GTK2.TreeViewColumn column );
//! Appends column to the list of columns.  If this tree view has
//! "fixed_height" mode enabled, then column must have its "sizing" property
//! set to be GTK2.TREE_VIEW_COLUMN_FIXED.
//!
//!

GTK2.TreeView collapse_all( );
//! Recursively collapses all visible, expanded nodes.
//!
//!

int collapse_row( GTK2.TreePath path );
//! Collapses a row (hides its child rows, if they exist).
//!
//!

GTK2.TreeView columns_autosize( );
//! Resizes all columns to their optimal width.  Only works after the treeview
//! has been realized.
//!
//!

static GTK2.TreeView create( GTK2.TreeModel model_or_props );
//! Create a new W(TreeView), with or without a default model.
//!
//!

GTK2.TreeView expand_all( );
//! Recursively expands all nodes.
//!
//!

int expand_row( GTK2.TreePath path, int open_all );
//! Opens the row so its children are visible.
//!
//!

GTK2.TreeView expand_to_path( GTK2.TreePath path );
//! Expands the row at path.  This will also expand all parent rows of path as
//! necessary.
//!
//!

GTK2.GdkRectangle get_background_area( GTK2.TreePath path, GTK2.TreeViewColumn column );
//! Similar to get_cell_area().  The returned rectangle is equivalent to the
//! background_area passed to GTK2.CellRenderer->render().  These background
//! area tiles to cover the entire tree window (except for the area used for
//! header buttons).  Contrast with get_cell_area(), which returns only the
//! cell itself, excluding surrounding borders and the tree expander area.
//!
//!

GTK2.GdkWindow get_bin_window( );
//! Returns the window that this view renders to.  This is used primarily to
//! compare to event->window to confirm that the event on this view is on the
//! right window.
//!
//!

GTK2.GdkRectangle get_cell_area( GTK2.TreePath path, GTK2.TreeViewColumn column );
//! Fills the bounding rectangle in tree window coordinates for the cell at
//! the row specified by path and the column specified by column.  If path is
//! omitted or 0, or points to a path not currently displayed, the y and
//! height fields of the rectangle will be 0.  If column is omitted, the x and
//! width fields will be o.  The sum of all cell rects does not cover the
//! entire tree; there are extra pixels in between rows, for example.  The
//! returned rectangle is equivalent to the cell_area passed to
//! GTK2.CellRenderer->render().  This function is only valid if the view is
//! realized.
//!
//!

GTK2.TreeViewColumn get_column( int n );
//! Gets the W(TreeViewColumn) at the given position.
//!
//!

array get_columns( );
//! Returns an array of all the W(TreeViewColumn)'s current in the view.
//!
//!

mapping get_cursor( );
//! Returns the current path and focus column.  If the cursor isn't currently
//! set, then "path" will be 0.  If no column currently has focus, then
//! "focus_column" will be 0.
//! Returns ([ "path": GTK2.TreePath, "column": GTK2.TreeViewColumn ]);
//!
//!

int get_enable_search( );
//! Returns whether or not the tree allows to start interactive searching by
//! typing in text.
//!
//!

GTK2.TreeViewColumn get_expander_column( );
//! Returns the column that is the current expander column.  This column has
//! the expander arrow drawn next to it.
//!
//!

int get_fixed_height_mode( );
//! Returns whether fixed height mode is turned on.
//!
//!

GTK2.Adjustment get_hadjustment( );
//! Gets the W(Adjustment) currently being used for the horizontal aspect.
//!
//!

int get_headers_visible( );
//! Returns true if the headers are visible.
//!
//!

int get_hover_expand( );
//! Returns whether hover expansion mode is turned on.
//!
//!

int get_hover_selection( );
//! Returns whether hover selection mode is turned on.
//!
//!

GTK2.TreeModel get_model( );
//! Returns the model this TreeView is based on.
//!
//!

mapping get_path_at_pos( int x, int y );
//! Finds the path at the point (x,y) relative to widget coordinates.  That
//! is, x and y are relative to an events coordinates.  x and y must come from
//! an event on the view only where event->window==get_bin().  It is primarily
//! for things like popup menus.  Returns GTK2.TreePath, GTK2.TreeViewColumn,
//! and cell_x and cell_y, which are the coordinates relative to the cell
//! background (i.e. the background_area passed to GTK2.CellRenderer->render()).
//! This function is only meaningful if the widget is realized.
//!
//!

int get_reorderable( );
//! Retrieves whether the user can reorder the tree via drag-and-drop.
//!
//!

int get_rules_hint( );
//! Gets the setting set by set_rules_hint().
//!
//!

int get_search_column( );
//! Gets the column searched on by the interactive search code.
//!
//!

array get_selected( );
//! Shortcut to GTK2.TreeView->get_selection() and 
//! GTK2.TreeSelection()->get_selected().
//!
//!

GTK2.TreeSelection get_selection( );
//! Gets the W(TreeSelection) associated with this TreeView.
//!
//!

GTK2.Adjustment get_vadjustment( );
//! Gets the W(Adjustment) currently being used for the vertical aspect.
//!
//!

array get_visible_range( );
//! Returns the first and last visible path.  Note that there may be invisible
//! paths in between.
//!
//!

GTK2.GdkRectangle get_visible_rect( );
//! Returns a GDK2.Rectangle with the currently-visible region of the buffer,
//! in tree coordinates.  Conver to widget coordinates with 
//! tree_to_widget_coords().  Tree coordinates start at 0,0 for row 0 of the
//! tree, and cover the entire scrollable area of the tree.
//!
//!

int insert_column( GTK2.TreeViewColumn column, int position );
//! This inserts the column at position.  If position is -1, then the column
//! is inserted at the end.  If this tree view has "fixed_height" mode
//! enabled, then column must have its "sizing property set to
//! GTK2.TREE_VIEW_COLUMN_FIXED.
//!
//!

GTK2.TreeView move_column_after( GTK2.TreeViewColumn column, GTK2.TreeViewColumn base );
//! Moves column to be after base.  If base is omitted, then column is
//! placed in the first position.
//!
//!

int remove_column( GTK2.TreeViewColumn column );
//! Removes column.
//!
//!

GTK2.TreeView row_activated( GTK2.TreePath path, GTK2.TreeViewColumn column );
//! Activates the cell determined by path and column.
//!
//!

int row_expanded( GTK2.TreePath path );
//! Returns true if the node pointed to by path is expanded.
//!
//!

GTK2.TreeView scroll_to_cell( GTK2.TreePath path, GTK2.TreeViewColumn column, float|void row_align, float|void col_align );
//! Moves the alignments of the view to the position specified by column and
//! path.  If column is 0, then no horizontal scrolling occurs.  Likewise, if
//! path is 0, no vertical scrolling occurs.  At a minimum, one of column or 
//! path needs to be non-zero.  row_align determines where the row is placed,
//! and col_align determines where column is placed.  Both are expected to be
//! between 0.0 and 1.0.  0.0 means left/top alignment, 1.0 means right/bottom
//! alignment, 0.5 means center.
//! 
//! If row_align exists, then col_align must exist, otherwise neither will be
//! used.  If neither are used, the tree does the minimum amount of work to
//! scroll the cell onto the screen.  This means that the cell will be scrolled
//! to the edge closest to its current position.  If the cell is currently
//! visible on the screen, nothing is done.
//! 
//! This function only works if the model is set, and path is a valid row on
//! the model.  If the model changes before the view is realized, the centered
//! path will be modifed to reflect this change.
//!
//!

GTK2.TreeView scroll_to_point( int x, int y );
//! Scrolls the tree view such that the top-left corner of the visible area
//! is x,y, where x and y are specified in tree window coordinates.  The view
//! must be realized before this function is called.  If it isn't, you 
//! probably want to be using scroll_to_cell().
//! 
//! If either x or y are -1, then that direction isn't scrolled.
//!
//!

GTK2.TreeView set_cursor( GTK2.TreePath path, GTK2.TreeViewColumn focus_column, int|void start_editing );
//! Sets the current keyboard focus to be at path, and selects it.  This is
//! useful when you want to focus the user's attention on a particular row.
//! If focus_column is present, then focus is given to the column specified by
//! it.  Additionally, if focus_column is specified, and start_editing is
//! true, then editing should be started in the specified cell.  This function
//! is often followed by grab_focus() in order to give keyboard focus to the
//! widget.  Please note that editing can only happen when the widget is 
//! realized.
//!
//!

GTK2.TreeView set_cursor_on_cell( GTK2.TreePath path, GTK2.TreeViewColumn focus_column, int|void start_editing, GTK2.CellRenderer focus_cell );
//! Sets the current keyboard focus to be at path, and selects it.  This is
//! useful when you want to focus the user's attention on a particular row.
//! If focus_column is present, then focus is given to the column specified by
//! it.  If focus_column and focus_cell are present, and focus_column contains
//! 2 or more editable or activatable cells, then focus is given to the cell
//! specified by focus_cell.  Additionally, if focus_column is specified, and
//! start_editing is true, then editing should be started in the specified
//! cell.  This function is often followed by grab_focus() in order to give
//! keyboard focus to the widget.  Please note that editing can only happen
//! when the widget is realized.
//!
//!

GTK2.TreeView set_enable_search( int enable_search );
//! If enable_search is set, then the user can type in text to search through
//! the tree interactively (this is sometimes called "typeahead find").
//! 
//! Note that even if this is false, the user can still initiate a search
//! using the "start-interactive-search" key binding.
//!
//!

GTK2.TreeView set_expander_column( GTK2.TreeViewColumn column );
//! Sets the column to draw the expander arrow at.  It must be in the view.
//! If column is omitted, then the expander arrow is always at the first
//! visible column.
//!
//!

GTK2.TreeView set_fixed_height_mode( int enable );
//! Enables or disables the fixed height mode.  Fixed height mode speeds up
//! W(TreeView) by assuming that all rows have the same height.  Only enable
//! this option if all rows are the same height and all columns are of type
//! GTK2.TREE_VIEW_COLUMN_FIXED.
//!
//!

GTK2.TreeView set_hadjustment( GTK2.Adjustment hadj );
//! Sets the W(Adjustment) for the current horizontal aspect.
//!
//!

GTK2.TreeView set_headers_clickable( int setting );
//! Allow the column title buttons to be clicked.
//!
//!

GTK2.TreeView set_headers_visible( int headers_visible );
//! Sets the visibility state of the headers.
//!
//!

GTK2.TreeView set_hover_expand( int expand );
//! Enables or disables the hover expansion mode.  Hover expansion makes rows
//! expand or collapse if the pointer moves over them.
//!
//!

GTK2.TreeView set_hover_selection( int hover );
//! Enables or disables the hover selection mode.  Hover selection makes the
//! selected row follow the pointer.  Currently, this works only for the
//! selection modes GTK2.SELECTION_SINGLE and GTK2.SELECTION_BROWSE.
//!
//!

GTK2.TreeView set_model( GTK2.TreeModel model );
//! Sets the model.  If this TreeView already has a model set, it will remove
//! it before setting the new model.
//!
//!

GTK2.TreeView set_reorderable( int reorderable );
//! This function is a convenience function to allow you to reorder models.
//! If reorderable is true, then the user can reorder the model by dragging
//! and dropping rows.  The developer can listen to these changes by connecting
//! to the model's "row-inserted" and "row-deleted" signals.
//! 
//! This function does not give you any degree of control over the order --
//! any reordering is allowed.  If more control is needed, you should probably
//! handle drag and drop manually.
//!
//!

GTK2.TreeView set_rules_hint( int setting );
//! This function tells GTK2+ that the user interface for your application
//! requires users to read across tree rows and associate cells with one
//! another.  By default, GTK2+ will then render the tree with alternating row
//! colors.  Do not use it just because you prefer the appearance of the
//! ruled tree; that's a question for the theme.  Some themes will draw tree
//! rows in alternating colors even when rules are turned off, and users who
//! prefer that appearance all the time can choose those themes.  You should
//! call this function only as a semantic hint to the theme engine that your
//! tree makes alternating colors usefull from a functional standpoint
//! (since it has lots of columns, generally).
//!
//!

GTK2.TreeView set_search_column( int column );
//! Sets column as the column where the interactive search code should search
//! in.
//! 
//! If the sort column is set, users can use the "start-interactive-search"
//! key binding to bring up search popup.  The enable-search property controls
//! whether simply typing text will also start an interactive search.
//! 
//! Note that column refers to a column of the model.
//!
//!

GTK2.TreeView set_vadjustment( GTK2.Adjustment vadj );
//! Sets the W(Adjustment) for the current vertical aspect.
//!
//!

mapping tree_to_widget_coords( int tx, int ty );
//! Converts tree coordinates (coordinates in full scrollable area of the tree)
//! to widget coordinates.
//!
//!

mapping widget_to_tree_coords( int wx, int wy );
//! converts widget coordinates to coordinates for the tree window (the full
//! scrollable area of the tree).
//!
//!
