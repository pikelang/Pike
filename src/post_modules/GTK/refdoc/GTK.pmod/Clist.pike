//! The GtkCList widget is a multi-column list widget that is capable
//! of handling literally thousands of rows of information. Each column
//! can optionally have a title, which itself is optionally active,
//! allowing us to bind a function to it's selection.
//!@code{ GTK.Clist(2)->set_column_title(0,"Col 1")->set_column_title(1,"Col 2")->column_titles_show()->set_column_width(0,50)->set_usize(150,70)@}
//!@xml{<image>../images/gtk_clist.png</image>@}
//!
//!
//!
//!  Signals:
//! @b{abort_column_resize@}
//!
//! @b{click_column@}
//! Called when a column is clicked
//!
//!
//! @b{end_selection@}
//!
//! @b{extend_selection@}
//!
//! @b{resize_column@}
//! Called when a column is resized
//!
//!
//! @b{scroll_horizontal@}
//!
//! @b{scroll_vertical@}
//!
//! @b{select_all@}
//!
//! @b{select_row@}
//! Called when a row is selected
//!
//!
//! @b{start_selection@}
//!
//! @b{toggle_add_mode@}
//!
//! @b{toggle_focus_row@}
//!
//! @b{undo_selection@}
//!
//! @b{unselect_all@}
//!
//! @b{unselect_row@}
//! Called when a row is deselected
//!
//!

inherit GTK.Container;

int append( array columns );
//!  The return value of indicates the index of the row that was just
//!  added.
//! 
//! 'columns' are the texts we want to put in the columns. The size of
//! the array should equal the number of columns in the list.
//!
//!

GTK.Clist clear( );
//! remove all rows
//!
//!

GTK.Clist column_title_active( int column );
//! Make a specific column title active
//!
//!

GTK.Clist column_title_passive( int column );
//! Make a specific column title passive
//!
//!

GTK.Clist column_titles_active( );
//! The column titles can be pressed
//!
//!

GTK.Clist column_titles_hide( );
//! Hide the column titles
//!
//!

GTK.Clist column_titles_passive( );
//! The column titles can't be pressed
//!
//!

GTK.Clist column_titles_show( );
//! Show the column titles.
//!
//!

int columns_autosize( );
//! Resize all columns according to their contents
//!
//!

static GTK.Clist create( int columns );
//! Create a new empty clist, columns columns wide.
//! 
//! Not all columns have to be visible, some can be used to store data that is
//! related to a certain cell in the list.
//!
//!
//!

int find_row_from_data( object data );
//! Find a row in the list that has the given user data.  If no node is
//! found, 0 is returned.
//!
//!

GTK.Clist freeze( );
//! freeze all visual updates of the list, and then thaw the list after
//! you have made a number of changes and the updates wil occure in a
//! more efficent mannor than if you made them on a unfrozen list
//!
//!

GDK.Color get_background( int row );
//! Return the background color of a specified row
//!
//!

GTK.Style get_cell_style( int row, int col );
//! return the W(Style) associated with a specific cell
//!
//!

int get_cell_type( int row, int column );
//! Return value is one of @[CELL_EMPTY], @[CELL_PIXMAP], @[CELL_PIXTEXT], @[CELL_TEXT] and @[CELL_WIDGET]
//!
//!

string get_column_title( int column );
//! Returns the title of a specified column.
//!
//!

GTK.Widget get_column_widget( int column );
//! Return the widget for the specified column title
//!
//!

int get_columns( );
//! Return the number of columns in this clist
//!
//!

int get_drag_button( );
//! Return the button used to drag items (by default 1)
//!
//!

int get_flags( );
//! Return the flags. A bitwise or of @[CLIST_ADD_MODE], @[CLIST_AUTO_RESIZE_BLOCKED], @[CLIST_AUTO_SORT], @[CLIST_DRAW_DRAG_LINE], @[CLIST_DRAW_DRAG_RECT], @[CLIST_IN_DRAG], @[CLIST_REORDERABLE], @[CLIST_ROW_HEIGHT_SET], @[CLIST_SHOW_TITLES] and @[CLIST_USE_DRAG_ICONS]
//!
//!

int get_focus_row( );
//! The currently focused row
//!
//!

GDK.Color get_foreground( int row );
//! Return the foregroun color for the specified row
//!
//!

GTK.Adjustment get_hadjustment( );
//! Return the W(Adjustment) object used for horizontal scrolling
//!
//!

GTK.Clist get_pixmap( int row, int column );
//! Return the pixmap for the specified cell
//!
//!

mapping get_pixtext( int row, int col );
//! Return the pixmap and text for the specified cell as a mapping:
//! ([ "spacing":spacing, "text":text, "pixmap":pixmap ])
//!
//!

object get_row_data( int row );
//! Return the data associated with a row, or 0.
//!
//!

int get_row_height( );
//! Return the height of the row
//!
//!

GTK.Style get_row_style( int row );
//! Return the W(style) object associated with the specified row
//!
//!

int get_rows( );
//! Return the number of rows
//!
//!

int get_selectable( int row );
//! Return 1 if the specified row can be selected by the user.
//!
//!

array get_selection( );
//! Return an array with all selected rows.
//!
//!

mapping get_selection_info( int x, int y );
//! Return the row column corresponding to the x and y coordinates,
//! the returned values are only valid if the x and y coordinates
//! are relative to the clist window coordinates
//!
//!

int get_selection_mode( );
//! Return the selection mode. One of @[SELECTION_BROWSE], @[SELECTION_EXTENDED], @[SELECTION_MULTIPLE] and @[SELECTION_SINGLE]
//!
//!

int get_shadow_type( );
//! Return the curreent shadow type. One of @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_OUT]
//!
//!

int get_sort_column( );
//! The column that will be used to sort the rows
//!
//!

int get_sort_type( );
//! The sort method, one of @[SORT_ASCENDING] and @[SORT_DESCENDING]
//!
//!

GTK.Clist get_text( int row, int col );
//! Return the text associated with a specific cell.
//!
//!

GTK.Adjustment get_vadjustment( );
//! Return the W(Adjustment) object used for vertical scrolling
//!
//!

int insert( int row, array columns );
//! Insert a row after a specified row.
//!  The return value of indicates the index of the row that was just
//!  added, please note that this is not nessasarily the same row as
//!  the specified one, if autosort is activated, the row will be
//!  inserted so that the list is sill sorted.
//! 
//! 'columns' are the texts we want to put in the columns. The size of
//! the array should equal the number of columns in the list.
//!
//!

GTK.Clist moveto( int row, int column, float xpos, float ypos );
//! Make the specified row and column visible, and place it relative to
//! xpos and ypos in the area the Clist occupies.   xpos and ypos
//! are relative, 0.0 == top, 1.0 == bottom
//!
//!

int optimal_column_width( int column );
//! Return the optimal width of the specified column
//!
//!

int prepend( array columns );
//!  The return value of indicates the index of the row that was just
//!  added.
//! 
//! 'columns' are the texts we want to put in the columns. The size of
//! the array should equal the number of columns in the list.
//!
//!

GTK.Clist remove( int row );
//! Delete a specified row. If you want to remove all rows in a Clist,
//! use 'clear()' instead of calling remove multiple times.
//!
//!

GTK.Clist row_move( int from_row, int to_row );
//! Move the specified row to just before the specified destination
//! row.
//!
//!

GTK.Clist select_all( );
//! Select all rows
//!
//!

GTK.Clist select_row( int row, int column );
//! Select the given row. The column is sent to the signal handler, but
//! ignored for all other purposes.
//!
//!

GTK.Clist set_auto_sort( int sortp );
//! If true, the clist will automatically be re-sorted when new rows
//! are inserted. Please note that it will not be resorted if the text
//! in cells are changed, use 'sort()' to force a reorder. The sort
//! function is stable.
//!
//!

GTK.Clist set_background( int row, GDK.Color color );
//! Set the background color of the specified row the the specified color
//!
//!

GTK.Clist set_button_actions( int button, int action );
//! Action is a bitwise or of @[BUTTON_DRAGS], @[BUTTON_EXPANDS], @[BUTTON_IGNORED], @[BUTTON_SELECTS], @[BUTTONBOX_DEFAULT_STYLE], @[BUTTONBOX_EDGE], @[BUTTONBOX_END], @[BUTTONBOX_SPREAD] and @[BUTTONBOX_START]
//! Button is the mouse button (normally 1-3, 4 and 5 sometimes beeing
//! scroll wheel up and scroll wheel down)
//!
//!

GTK.Clist set_cell_style( int row, int column, GTK.Style style );
//! Set a W(Style) for a specific cell
//!
//!

GTK.Clist set_column_auto_resize( int column, int autoresizep );
//! Automatically resize a column to the width of it's widest contents.
//!
//!

GTK.Clist set_column_justification( int column, int justification );
//! justification is one of @[JUSTIFY_CENTER], @[JUSTIFY_FILL], @[JUSTIFY_LEFT] and @[JUSTIFY_RIGHT]
//!
//!

GTK.Clist set_column_max_width( int column, int width );
//! if width 6lt; 0 , there is no restriction
//!
//!

GTK.Clist set_column_min_width( int column, int width );
//! Width in pixels
//!
//!

GTK.Clist set_column_resizeable( int column, int resizeablep );
//! Make a column resizable, or remove it's the resizability.
//!
//!

GTK.Clist set_column_title( int column, string title );
//! Set the column title of a specified column. It is a good idea to
//! set the titles before the column title buttons are shown.
//!
//!

GTK.Clist set_column_visibility( int column, int visiblep );
//! Hide or show a column
//!
//!

GTK.Clist set_column_widget( int column, GTK.Widget widget );
//! Put a widget as a column title. The widget will be added to a
//! W(Button).
//!
//!

GTK.Clist set_column_width( int column, int width );
//! Width in pixels
//!
//!

GTK.Clist set_compare_func( function cmpfun );
//! Set the compare function. The function will be called with a
//! mapping as it's only argument, like this:@pre{
//!   ([
//!      "clist":the clist widget,
//!      "sort_column":the column to sort on,
//!      "row1_data":The user data pointer for the first row,
//!      "row2_data":The user data pointer for the second row,
//!      "row1_text":The text in the sort cell in the first row
//!      "row2_text":The text in the sort cell in the second row
//!   ])
//! @}
//! The return value is one of:
//!   1: Row 1 is more than row 2
//!   0: The rows are equal
//!  -1: Row 1 is lesser than row 2
//! To remove the comparefunction, use 0 as the argument.
//!
//!

GTK.Clist set_foreground( int row, GDK.Color color );
//! Set the foreground color of the specified row to the specified color
//!
//!

GTK.Clist set_hadjustment( GTK.Adjustment adjustment );
//! Set the W(Adjustment) object used for horizontal scrolling
//!
//!

GTK.Clist set_pixmap( int row, int col, GDK.Pixmap image, GDK.Bitmap mask );
//! Set the pixmap of the specified cell. The mask is optional
//!
//!

GTK.Clist set_pixtext( int row, int column, string text, int spacing, GDK.Pixmap image, GDK.Bitmap mask );
//! Set the pixmap and text of the specified cell. The mask is optional
//! The spacing is the number of pixels between the pixmap and the text.
//!
//!

GTK.Clist set_reorderable( int reorderablep );
//! If true, the user can drag around the rows in the list.
//!
//!

GTK.Clist set_row_data( int row, object data );
//! Set the user data associated with the specified row.
//! This data can be used to find rows, and when a row is selected it
//! can be easily retrieved using node_get_row_data.
//! 
//! @b{You can only use objects as row data right now@}
//!
//!

GTK.Clist set_row_height( int pixels );
//! in pixels
//!
//!

GTK.Clist set_row_style( int row, GTK.Style style );
//!

GTK.Clist set_selectable( int row, int selectablep );
//! If true, the row can be selected by the user, otherwise it cannot
//! be selected, only focused.
//!
//!

GTK.Clist set_selection_mode( int mode );
//! One of @[SELECTION_BROWSE], @[SELECTION_EXTENDED], @[SELECTION_MULTIPLE] and @[SELECTION_SINGLE]
//!
//!

GTK.Clist set_shadow_type( int shadowtype );
//! One of @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_OUT]
//!
//!

GTK.Clist set_shift( int row, int column, int yshift, int xshift );
//! The contents of the specified cell will be drawn shifted (indented)
//! the specifid number of pixels.
//! 
//! This can be useful to generate a tree-like layout when you do not
//! want to make a W(Ctree)
//! 
//!
//!

GTK.Clist set_sort_column( int column );
//!

GTK.Clist set_sort_type( int direction );
//! Ascending or descending (One of @[SORT_ASCENDING] and @[SORT_DESCENDING])
//!
//!

GTK.Clist set_text( int row, int column, string text );
//! Set the text for the specified cell. Please note that even if auto
//! sorting is enabled, the row will not be resorted. Use the 'sort()'
//! function.
//!
//!

GTK.Clist set_use_drag_icons( int dragiconsp );
//! If true, hard coded drag icons will be used.
//!
//!

GTK.Clist set_vadjustment( GTK.Adjustment adjustment );
//! Set the W(Adjustment) object used for vertical scrolling
//!
//!

GTK.Clist sort( );
//! Set the column on which all sorting will be performed
//!
//!

GTK.Clist thaw( );
//! freeze all visual updates of the list, and then thaw the list after
//! you have made a number of changes and the updates wil occure in a
//! more efficent mannor than if you made them on a unfrozen list
//!
//!

GTK.Clist undo_selection( );
//! Undo the previous selection
//!
//!

GTK.Clist unselect_all( );
//! Unselect all rows
//!
//!

GTK.Clist unselect_row( int row, int column );
//! Unselect the given row. The column is sent to the signal handler,
//! but ignored for all other purposes.
//!
//!
