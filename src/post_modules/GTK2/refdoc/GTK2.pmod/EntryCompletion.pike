// Automatically generated from "gtkentrycompletion.pre".
// Do NOT edit.

//! Properties:
//! int inline-completion
//! int minimum-key-length
//! GTK2.TreeModel model
//! int popup-completion
//! int popup-single-match
//! int text-column
//!
//!
//!  Signals:
//! @b{action_activated@}
//!
//! @b{insert_prefix@}
//!
//! @b{match_selected@}
//!

inherit G.Object;
//!

inherit GTK2.CellLayout;
//!

GTK2.EntryCompletion complete( );
//! Requests a completion operation, or in other words a refiltering of the
//! current list with completions, using the current key.  The completion list
//! view will be updated accordingly.
//!
//!

protected void create( void props );
//! Creates a new widget.
//!
//!

GTK2.EntryCompletion delete_action( int index );
//! Deletes the action at index.
//!
//!

GTK2.Widget get_entry( );
//! Gets the entry this widget has been attached to.
//!
//!

int get_inline_completion( );
//! Returns whether the common prefix of the possible completions should be
//! automatically inserted in the entry.
//!
//!

int get_minimum_key_length( );
//! Returns the minimum key length.
//!
//!

GTK2.TreeModel get_model( );
//! Returns the model being used as the data source.
//!
//!

int get_popup_completion( );
//! Returns whether completions should be presented in a popup window.
//!
//!

int get_popup_set_width( );
//! Returns whether the completion popup window will be resized to the width
//! of the entry.
//!
//!

int get_popup_single_match( );
//! Returns whether the completion popup window will appear even if there is
//! only a single match.
//!
//!

int get_text_column( );
//! Returns the column in the model to get strings from.
//!
//!

GTK2.EntryCompletion insert_action_markup( int index, string markup );
//! Inserts an action in the action item list at position index with the
//! specified markup.
//!
//!

GTK2.EntryCompletion insert_action_text( int index, string text );
//! Inserts an action in the action item list at position index with the
//! specified text.  If you want the action item to have markup, use
//! insert_action_markup().
//!
//!

GTK2.EntryCompletion insert_prefix( );
//! Requests a prefix insertion.
//!
//!

GTK2.EntryCompletion set_inline_completion( int inline_completion );
//! Sets whether the common prefix of the possible completions should be
//! automatically inserted in the entry.
//!
//!

GTK2.EntryCompletion set_inline_selection( int inline_selection );
//! Sets whether it is possible to cycle through the possible completions inside the entry.
//!
//!

GTK2.EntryCompletion set_match_func( function cb );
//! Sets the function to be called to decide if a specific row should
//! be displayed.
//!
//!

GTK2.EntryCompletion set_minimum_key_length( int length );
//! Requires the length of the search key to be at least length long.  This is
//! useful for long lists, where completing using a small key takes a lot of
//! time and will come up with meaningless results anyway (i.e. a too large
//! dataset).
//!
//!

GTK2.EntryCompletion set_model( GTK2.TreeModel model );
//! Sets the model.  If this completion already has a model set, it will
//! remove it before setting the new model.  If omitted it will unset the
//! model.
//!
//!

GTK2.EntryCompletion set_popup_completion( int setting );
//! Sets whether the completions should be presented in a popup window.
//!
//!

GTK2.EntryCompletion set_popup_set_width( int setting );
//! Sets whether the completion popup window will be resized to be the same
//! width as the entry.
//!
//!

GTK2.EntryCompletion set_popup_single_match( int setting );
//! Sets whether the completion popup window will appear even if there is only
//! a single match.  You may want to set this to 0 if you are using inline
//! completion.
//!
//!

GTK2.EntryCompletion set_text_column( int column );
//! Convenience function for setting up the most used case of this code: a
//! completion list with just strings.  This function will set up the
//! completion to have a list displaying all (and just) strings in the list,
//! and to get those strings from column column in the model.
//!
//!
