//! Properties:
//! int active
//! int add-tearoffs
//! int column-span-column
//! int focus-on-click
//! int has-frame
//! GTK2.TreeModel model
//! int row-span-column
//! int wrap-width
//! 
//! Style properties:
//! int appears-as-list
//!
//!
//!  Signals:
//! @b{changed@}
//!

inherit GTK2.Bin;

inherit GTK2.CellLayout;

inherit GTK2.CellEditable;

GTK2.ComboBox append_text( string text );
//! Appends text to the list of strings stored in this combo box.
//! Note that you can only use this function with combo boxes
//! constructed with GTK2.ComboBox("a string").
//!
//!

static GTK2.ComboBox create( GTK2.TreeModel model_or_props );
//! Create a new ComboBox, either empty or with a model.  If a string is passed
//! int instead, it will create a new W(ComboBox) with only text strings.
//! If you do so, you should only manipulate it with the following functions:
//! append_text(), insert_text(), prepend_text(), and remove_text().
//!
//!

int get_active( );
//! Returns the index of the currently active item, or -1 if none.
//! If the model is a non-flat treemodel, and the active item
//! is not an immediate child of the root of the tree, this
//! function returns path_get_indices(path)[0], where path is
//! the GTK2.TreePath of the active item.
//!
//!

GTK2.TreeIter get_active_iter( );
//! Get the current active item.
//!
//!

string get_active_text( );
//! Returns the currently active string.
//! Note that you can only use this function with combo boxes
//! constructed with GTK2.ComboBox("a string").
//!
//!

int get_add_tearoffs( );
//! Gets whether the popup menu has tearoff items.
//!
//!

int get_column_span_column( );
//! Returns the column with column span information.
//!
//!

int get_focus_on_click( );
//! Returns whether the combo box grabs focus when it is
//! clicked with the mouse.
//!
//!

GTK2.TreeModel get_model( );
//! Get the GTK2.TreeModel which is acting as a data source.
//!
//!

int get_row_span_column( );
//! Returns the column with row span information.
//!
//!

int get_wrap_width( );
//! Returns the wrap width which is used to determine
//! the number of columns for the popup menu.  If the wrap
//! width is larger than 1, the combo box is in table mode.
//!
//!

GTK2.ComboBox insert_text( int position, string text );
//! Inserts string at position in the list of strings stored.
//! Note that you can only use this function with combo boxes
//! constructed with GTK2.ComboBox("a string").
//!
//!

GTK2.ComboBox popdown( );
//! Hides the menu or dropdown list.
//!
//!

GTK2.ComboBox popup( );
//! Pops up the menu or dropdown list.
//!
//!

GTK2.ComboBox prepend_text( string text );
//! Prepends string to the list of strings stored in this combo box.
//! Note that you can only use this function with combo boxes
//! constructed with GTK2.ComboBox("a string").
//!
//!

GTK2.ComboBox remove_text( int position );
//! Removes the string at position from this combo box.
//! Note that you can only use this function with combo boxes
//! constructed with GTK2.ComboBox("a string").
//!
//!

GTK2.ComboBox set_active( int index_ );
//! Sets the active item.
//!
//!

GTK2.ComboBox set_active_iter( GTK2.TreeIter iter );
//! Sets the current active item to be the one referenced by iter.
//! iter must correspond to a path of depth one.
//!
//!

GTK2.ComboBox set_add_tearoffs( int setting );
//! Sets whether the popup menu should have a tearoff menu item.
//!
//!

GTK2.ComboBox set_column_span_column( int column_span );
//! Sets the column span information.  The column span column
//! contains integers which indicate how many columns
//! an item should span.
//!
//!

GTK2.ComboBox set_focus_on_click( int setting );
//! Sets whether the combo box will grab focus when it is
//! clicked with the mouse.
//!
//!

GTK2.ComboBox set_model( GTK2.TreeModel model );
//! Sets the model used by this widget.  Will unset a previously
//! set model.  If no arguments are passed, then it will unset
//! the model.
//!
//!

GTK2.ComboBox set_row_separator_func( function f, mixed user_data );
//! Sets the row separator function, which is used to determine whether a
//! row should be drawn as a separator.  If the row separator function is 0
//! no separators are drawn.  This is the default value.
//!
//!

GTK2.ComboBox set_row_span_column( int row_span );
//! Sets the column with row span information.  The row span column
//! contains integers which indicate how many rows an item
//! should span.
//!
//!

GTK2.ComboBox set_wrap_width( int width );
//! Sets the wrap width.  The wrap width is basically the preferred
//! number of columns when you want the popup to be layed out in
//! a table.
//!
//!
