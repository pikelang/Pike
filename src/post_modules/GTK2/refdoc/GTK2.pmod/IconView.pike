//! GTK2.IconView provides an alternative view on a list model.  It
//! displays the model as a grid of icons with labels.  Like GTK2.TreeView,
//! it allows to select one or multiple items (depending on the selection
//! mode).  In addition to seleciton with the arrow keys, GTK2.IconView
//! supports rubberband selections, which is controlled by dragging the
//! pointer.
//! Properties:
//! int column-spacing
//! int columns
//! int item-width
//! int margin
//! int markup-column
//! GTK2.TreeModel model
//! int orientation
//! int pixbuf-column
//! int row-spacing
//! int selection-mode
//! int spacing
//! int text-column
//! 
//! Style properties:
//! int selection-box-alpha
//! GDK2.Color selection-box-color
//!
//!
//!  Signals:
//! @b{activate_cursor_item@}
//!
//! @b{item_activated@}
//!
//! @b{move_cursor@}
//!
//! @b{select_all@}
//!
//! @b{select_cursor_item@}
//!
//! @b{selection_changed@}
//!
//! @b{set_scroll_adjustments@}
//!
//! @b{toggle_cursor_item@}
//!
//! @b{unselect_all@}
//!

inherit GTK2.Container;

static GTK2.IconView create( GTK2.TreeModel model_or_props );
//! Creates a new GTK2.IconView widget
//! Not implemented yet.
//!
//!

int get_column_spacing( );
//! Returns the value of the column-spacing property.
//!
//!

int get_columns( );
//! Returns the value of the columns property.
//!
//!

array get_cursor( );
//! Returns the GTK2.TreePath and GTK2.CellRenderer of the current cursor path
//! and cell.  If the cursor isn't currently set, then path will be 0.  If no
//! cell currently has focus, then cell will be 0.
//!
//!

array get_item_at_pos( int x, int y );
//! Finds the path at the point (x,y) relative to widget coordinates.  In
//! contrast to get_path_at_pos(), this function also obtains the cell at the
//! specified position.
//!
//!

int get_item_width( );
//! Returns the value of the item-width property.
//!
//!

int get_margin( );
//! Returns the value of the margin property.
//!
//!

int get_markup_column( );
//! Returns the column with markup text.
//!
//!

GTK2.TreeModel get_model( );
//! Gets the model.
//!
//!

int get_orientation( );
//! Returns the value of the orientation property.
//!
//!

GTK2.TreePath get_path_at_pos( int x, int y );
//! Finds the path at the point(x,y) relative to widget coordinates.
//!
//!

int get_pixbuf_column( );
//! Returns the column with pixbufs.
//!
//!

int get_reorderable( );
//! Retrieves whether the user can reorder the list via drag-and-drop.
//!
//!

int get_row_spacing( );
//! Returns the value of the row-spacing property.
//!
//!

array get_selected_items( );
//! Creates a list of paths of all selected items.
//! Not implemented yet.
//!
//!

int get_selection_mode( );
//! Gets the selection mode.
//!
//!

int get_spacing( );
//! Returns the value of the spacing property
//!
//!

int get_text_column( );
//! Returns the column with text.
//!
//!

array get_visible_range( );
//! Returns the first and last visible path.  Note that there may be invisible
//! paths in between.
//!
//!

GTK2.IconView item_activated( GTK2.TreePath path );
//! Activates the item determined by path.
//!
//!

int path_is_selected( GTK2.TreePath path );
//! Returns true if the icon pointed to by path is currently selected.
//! If icon does not point to a valid location, false is returned.
//!
//!

GTK2.IconView scroll_to_path( GTK2.TreePath path, int use_align, float row_align, float col_align );
//! Moves the alignments to the position specified by path.  row_align
//! determines where the row is placed, and col_align determines where column
//! is placed.  Both are expected to be between 0.0 and 1.0.  0.0 means
//! left/top alignment, 1.0 means right/bottom alignment, 0.5 means center.
//! 
//! If use_align is FALSE, then the alignment arguments are ignored, and the
//! tree does the minimum amount of work to scroll the item onto the screen.
//! This means that the item will be scrolled to the edge closest to its
//! current position.  If the item is currently visible on the screen, nothing
//! is done.
//! 
//! This funciton only works if the model is set, and path is a valid row on
//! the model.  If the model changes before this icon view is realized, the
//! centered path will be modified to reflect this change.
//!
//!

GTK2.IconView select_all( );
//! Selects all the icons.  This widget must have its selection
//! mode set to GTK2.SELECTION_MULTIPLE.
//!
//!

GTK2.IconView select_path( GTK2.TreePath path );
//! Selects the row at path
//!
//!

GTK2.IconView set_column_spacing( int column_spacing );
//! Sets the column-spacing property which specifies the space which is
//! inserted between the columns of the icon view.
//!
//!

GTK2.IconView set_columns( int columns );
//! Sets the columns property which determines in how many columns the
//! icons are arranged.  If columns is -1, the number of columns will
//! be chosen automatically to fill the available area.
//!
//!

GTK2.IconView set_cursor( GTK2.TreePath path, GTK2.CellRenderer cell, int|void start_editing );
//! Sets the current keyboard focus to be at path, and selects it.  This is
//! usefull when you want to focus the user's attention on a particular item.
//! If cell is not 0, then focus is given to the cell speicified by it.
//! Additionally, if start_editing is TRUE, then editing should be started in
//! the specified cell.
//! 
//! This function is often followed by grab_focus() in order to give keyboard
//! focus to the widget.
//!
//!

GTK2.IconView set_item_width( int item_width );
//! Sets the item-width property which specifies the width to use for
//! each item.  If it is set to -1, the icon view will automatically
//! determine a suitable item size.
//!
//!

GTK2.IconView set_margin( int margin );
//! Sets the margin property.
//!
//!

GTK2.IconView set_markup_column( int column );
//! Sets the column with markup information to be column.
//!
//!

GTK2.IconView set_model( GTK2.TreeModel model );
//! Sets the model.
//!
//!

GTK2.IconView set_orientation( int orientation );
//! Sets the orientation property which determines whether the labels
//! are drawn beside the icons instead of below.
//! One of @[ORIENTATION_HORIZONTAL] and @[ORIENTATION_VERTICAL]
//!
//!

GTK2.IconView set_pixbuf_column( int column );
//! Sets the column with pixbufs to be column.
//!
//!

GTK2.IconView set_reorderable( int setting );
//! This function is a convenience function to allow you to reorder models.
//! Both GTK2.TreeStore and GTK2.ListStore support this.  If setting is TRUE,
//! then the user can reorder the model by dragging and dropping rows.  The
//! developer can listen to these changes by connecting to the model's
//! "row-inserted" and "row-deleted" signals.
//!
//!

GTK2.IconView set_row_spacing( int row_spacing );
//! Sets the row-spacing property which specifies the space which is
//! inserted between the rows of the icon view.
//!
//!

GTK2.IconView set_selection_mode( int mode );
//! Sets the selection mode.
//! One of @[SELECTION_BROWSE], @[SELECTION_MULTIPLE], @[SELECTION_NONE] and @[SELECTION_SINGLE]
//!
//!

GTK2.IconView set_spacing( int spacing );
//! Sets the spacing property which specifies the space which is inserted
//! between the cells (i.e. the icon and the text) of an item.
//!
//!

GTK2.IconView set_text_column( int column );
//! Sets the column with text to be column.
//!
//!

GTK2.IconView unselect_all( );
//! Unselects all the icons.
//!
//!

GTK2.IconView unselect_path( GTK2.TreePath path );
//! Unselects the row at path
//!
//!
