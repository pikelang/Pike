// Automatically generated from "gtktreeviewcolumn.pre".
// Do NOT edit.

//! Properties:
//! float alignment
//! int clickable
//! int expand
//! int fixed-width
//! int max-width
//! int min-width
//! int reorderable
//! int resizable
//! int sizing @[TREE_VIEW_COLUMN_AUTOSIZE], @[TREE_VIEW_COLUMN_FIXED] and @[TREE_VIEW_COLUMN_GROW_ONLY]
//! int sort-indicator
//! int sort-order @[SORT_ASCENDING] and @[SORT_DESCENDING]
//! int spacing
//! string title
//! int visible
//! GTK2.Widget widget
//! int width
//!
//!
//!  Signals:
//! @b{clicked@}
//!

inherit GTK2.Data;
//!

inherit GTK2.CellLayout;
//!

GTK2.TreeViewColumn add_attribute( GTK2.CellRenderer cell_renderer, string attribute, int column );
//! Adds an attribute mapping to the list.  The column is the column of the
//! model to get a value from, and the attribute is the parameter on
//! cell_rendere to be set from the value.  So for example if column 2 of
//! the model contains strings, you could have the "text" attribute of a
//! W(CellRendererText) get its values from column 2.
//!
//!

mapping cell_get_position( GTK2.CellRenderer cell_renderer );
//! Obtains the horizontal position and size of a cell in a column.  If the
//! cell is not found in the column, returns -1 for start_pos and width.
//!
//!

mapping cell_get_size( GDK2.Rectangle cell_area );
//! Obtains the width and height needed to render the column.
//!
//!

int cell_is_visible( );
//! Returns true if any of the cells packed into the colum are visible.  For
//! this to be meaningful, you must first initialize the cells with
//! cell_set_cell_data().
//!
//!

GTK2.TreeViewColumn cell_set_cell_data( GTK2.TreeModel model, GTK2.TreeIter iter, int is_expander, int is_expanded );
//! Sets the cell renderer based on the model and iter.  That is, for every
//! attribute mapping in this column, it will get a value from the set column
//! on the iter, and use that value to set the attribute on the cell renderer.
//! This is used primarily by the W(TreeView).
//!
//!

GTK2.TreeViewColumn clear( );
//! Unsets all the mappings on all renderers.
//!
//!

GTK2.TreeViewColumn clear_attributes( GTK2.CellRenderer cell_renderer );
//! Clears all existing attributes previously set.
//!
//!

GTK2.TreeViewColumn clicked( );
//! Emits the "clicked" signal on the column.  This function will only work
//! if this column is clickable.
//!
//!

protected void create( void title_or_props, void renderer, void property, void col, void moreprops );
//! Creates a new W(TreeViewColumn). At least one property/col pair must be
//! specified; any number of additional pairs can also be given.
//!
//!

GTK2.TreeViewColumn focus_cell( GTK2.CellRenderer cell );
//! Sets the current keyboard focus to be at cell, if the column contains 2
//! or more editable and activatable cells.
//!
//!

float get_alignment( );
//! Returns the current x alignment.  This value can range between 0.0 and 1.0.
//!
//!

array get_cell_renderers( );
//! Returns an array of all the cell renderers in the column, in no partciular
//! order.
//!
//!

int get_clickable( );
//! Returns true if the user can click on the header for the column.
//!
//!

int get_expand( );
//! Return true if the column expands to take any available space.
//!
//!

int get_fixed_width( );
//! Gets the fixed width of the column.
//!
//!

int get_max_width( );
//! Returns the maximum width in pixels, or -1 if no maximum width is set.
//!
//!

int get_min_width( );
//! Returns the minimum width in pixels, or -1 if no minimum width is set.
//!
//!

int get_reorderable( );
//! Returns true if the column can be reordered by the user.
//!
//!

int get_resizable( );
//! Returns true if the column can be resized by the end user.
//!
//!

int get_sizing( );
//! Returns the current type.
//!
//!

int get_sort_column_id( );
//! Gets the logical column_id that the model sorts on when this column is
//! selected for sorting.
//!
//!

int get_sort_indicator( );
//! Gets the value set by set_sort_indicator();
//!
//!

int get_sort_order( );
//! Gets the value set by set_sort_order().
//!
//!

int get_spacing( );
//! Returns the spacing.
//!
//!

string get_title( );
//! Returns the title of the widget.
//!
//!

GTK2.Widget get_tree_view( );
//! Returns the W(TreeView) that this TreeViewColumn has been inserted into.
//!
//!

int get_visible( );
//! Returns true if the column is visible.
//!
//!

GTK2.Widget get_widget( );
//! Returns the W(Widget) in the button on the column header.  If a custom
//! widget has not been set then 0 is returned.
//!
//!

int get_width( );
//! Returns the current size in pixels.
//!
//!

GTK2.TreeViewColumn pack_end( GTK2.CellRenderer cell, int expand );
//! Packs the cell to the end of the column.
//!
//!

GTK2.TreeViewColumn pack_start( GTK2.CellRenderer cell, int expand );
//! Packs the cell into the beginning of the column.  If expand is false, then
//! the cell is allocated no more space than it needs.  Any unused space is
//! divied evenly between cells for which expand is true.
//!
//!

GTK2.TreeViewColumn queue_resize( );
//! Flags the column, and the cell renderers added to this column, to have
//! their sizes renegotiated.
//!
//!

GTK2.TreeViewColumn set_alignment( float xalign );
//! Sets the alignment of the title or custom widget inside the column header.
//! The alignment determines its location inside the button - 0.0 for left,
//! 0.5 for center, 1.0 for right.
//!
//!

GTK2.TreeViewColumn set_clickable( int clickable );
//! Sets the header to be active if clickable is true.  When the header is
//! active, then it can take keyboard focus, and can be clicked.
//!
//!

GTK2.TreeViewColumn set_expand( int expand );
//! Sets the column to take available extra space.  This space is shared
//! equally amongst all columns that have the expand set to true.  If no
//! column has this option set, then the last column gets all extra space.
//! By default, every column is created with this false.
//!
//!

GTK2.TreeViewColumn set_fixed_width( int fixed_width );
//! Sets the size of the column in pixels.  The is meaningful only if the
//! sizing type is GTK2.TREE_VIEW_COLUMN_FIXED.  The size of the column is
//! clamped to the min/max width for the column.  Please note that the
//! min/max width of the column doesn't actually affect the "fixed-width"
//! property of the widget, just the actual size when displayed.
//!
//!

GTK2.TreeViewColumn set_max_width( int max_width );
//! Sets the maximum width.  If max_width is -1, then the maximum width is
//! unset.  Note, the column can actually be wider than max width if it's the
//! last column in a view.  In this case, the column expands to fill any
//! extra space.
//!
//!

GTK2.TreeViewColumn set_min_width( int min_width );
//! Sets the minimum width.  If min_width is -1, then the minimum width is
//! unset.
//!
//!

GTK2.TreeViewColumn set_reorderable( int reorderable );
//! If reorderable is true, then the column can be reorderd by the end user
//! dragging the header.
//!
//!

GTK2.TreeViewColumn set_resizable( int resizable );
//! If resizable is true, then the user can explicitly resize the column by
//! grabbing the outer edge of the column button.  If resizable is ture and
//! the sizing mode of the column is GTK2.TREE_VIEW_COLUMN_AUTOSIZE, then the
//! sizing mode is changed to GTK2.TREE_VIEW_COLUMN_GROW_ONLY.
//!
//!

GTK2.TreeViewColumn set_sizing( int type );
//! Sets the growth behavior of this columnt to type.
//! One of @[TREE_VIEW_COLUMN_AUTOSIZE], @[TREE_VIEW_COLUMN_FIXED] and @[TREE_VIEW_COLUMN_GROW_ONLY].
//!
//!

GTK2.TreeViewColumn set_sort_column_id( int column_id );
//! Sets the logical column_id that this column sorts on when this column is
//! selected for sorting.  Doing so makes the column header clickable.
//!
//!

GTK2.TreeViewColumn set_sort_indicator( int setting );
//! Call this function with a setting of true to display an arrow in the
//! header button indicating the column is sorted.  Call set_sort_order()
//! to change the direction of the arrow.
//!
//!

GTK2.TreeViewColumn set_sort_order( int order );
//! Changes the appearance of the sort indicator.
//! 
//! This does not actually sort the model.  Use set_sort_column_id() if you
//! wnat automatic sorting support.  This function is primarily for custom
//! sorting behavior, and should be used in conjunction with 
//! GTK2.TreeSortable->set_sort_column() to do that.  For custom models, the
//! mechanism will vary.
//! 
//! The sort indicator changes direction to indicate normal sort or reverse
//! sort.  Note that you must have the sort indicator enabled to see anything
//! when calling this function.
//! One of @[SORT_ASCENDING] and @[SORT_DESCENDING].
//!
//!

GTK2.TreeViewColumn set_spacing( int spacing );
//! Sets the spacing field, which is the number of pixels to place between
//! cell renderers packed into it.
//!
//!

GTK2.TreeViewColumn set_title( string title );
//! Sets the title.  If a custom widget has been set, then this value is
//! ignored.
//!
//!

GTK2.TreeViewColumn set_visible( int visible );
//! Sets the visibility.
//!
//!

GTK2.TreeViewColumn set_widget( GTK2.Widget widget );
//! Sets the widget in the header to be widget.  If widget is omitted, then
//! the header button is set with a W(Label) set to the title.
//!
//!
