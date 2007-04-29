//!

GTK2.Editable copy_clipboard( );
//! Causes the characters in the current selection to be copied to the
//! clipboard.
//!
//!

GTK2.Editable cut_clipboard( );
//! Causes the characters in the current selection to be copied to the
//! clipboard and then deleted.
//!
//!

GTK2.Editable delete_selection( );
//! Deletes the current contents of the selection.
//!
//!

GTK2.Editable delete_text( int start, int end );
//! Deletes a sequence of characters.
//!
//!

string get_chars( int start, int end );
//! Retrieves a sequence of characters.
//!
//!

int get_editable( );
//! Retrieves whether this widget is editable.
//!
//!

int get_position( );
//! Retrieves the current cursor position.
//!
//!

array get_selection_bounds( );
//! Returns the selection bounds.
//!
//!

int insert_text( string text, int length, int pos );
//! Inserts text at a given position.  Returns the position after the new text.
//!
//!

GTK2.Editable paste_clipboard( );
//! Causes the contents of the clipboard to be pasted into the given widget at
//! the current cursor position.
//!
//!

GTK2.Editable select_region( int start, int end );
//! Selects a region of text.
//!
//!

GTK2.Editable set_editable( int setting );
//! Determines if the user can edit the text or not.
//!
//!

GTK2.Editable set_position( int pos );
//! Sets the cursor position.
//!
//!
