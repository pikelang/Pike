//! Text buffer object for GTK2.SourceView
//! 
//! Properties:
//! int check-brackets
//! int escape-char
//! int highlight
//! GTK2.SourceLanguage language
//! int max-undo-levels
//!
//!
//!  Signals:
//! @b{can_redo@}
//!
//! @b{can_undo@}
//!
//! @b{highlight_updated@}
//!
//! @b{marker_updated@}
//!

inherit GTK2.TextBuffer;

GTK2.SourceBuffer begin_not_undoable_action( );
//! Marks the beginning of a not undoable action on the buffer, disabling the
//! undo manager.  Typically you would call this function before initially
//! setting the contents of the buffer (e.g. when loading a file in a text
//! editor).
//! 
//! You may nest begin_no_undoable_action()/end_not_undoable_action() blocks.
//!
//!

int can_redo( );
//! Determines whether a source buffer can redo the last action.
//!
//!

int can_undo( );
//! Determines whether a source buffer can undo the last action.
//!
//!

protected GTK2.SourceBuffer create( GTK2.TextTagTable table_or_lang );
//! Create a new SourceBuffer.
//!
//!

GTK2.SourceBuffer end_not_undoable_action( );
//! Marks the end of a not undoable action on the buffer.  When the last not
//! undoable block is closed through a call to this function, the list of undo
//! actions is cleared and the undo manager is re-enabled.
//!
//!

GTK2.SourceLanguage get_language( );
//! Determines the GTK2.SourceLanguage used by the buffer.
//!
//!

int get_max_undo_levels( );
//! Determines the number of undo levels the buffer will track for buffer edits.
//!
//!

GTK2.SourceBuffer redo( );
//! Redoes the last undo operation.  Use can_redo() to check whether a call to
//! this function will have any effect.
//!
//!

GTK2.SourceBuffer set_language( GTK2.SourceLanguage lang );
//! Sets the source language the source buffer will use, adding GTK2.SourceTag
//! tags with the language's patterns and setting the escape character with
//! set_escape_char().  Note that this will remove any GTK2.SourceTag tags
//! currently in the buffer's tag table.
//!
//!

GTK2.SourceBuffer set_max_undo_levels( int setting );
//! Sets the number of undo levels for user actions the buffer will track.  If
//! the number of user actions exceeds the limit set by this funcction, older
//! actions will be discarded.
//! 
//! A new action is started whenever the function begin_user_action() is
//! called.  In general, this happens whenever the user presses any key which
//! modifies the buffer, but the undo manager will try to merge similar
//! consecutive actions, such as multiple character insertions into one action.
//! But, inserting a newline does start a new action.
//!
//!

GTK2.SourceBuffer undo( );
//! Undoes the last user action which modified the buffer.  Use can_undo() to
//! check whether a call to this function will have any effect.
//! 
//! Actions are defined as groups of operations between a call to
//! GTK2.TextBuffer->begin_user_action() and GTK2.TextBuffer->end_user_action(),
//! or sequences of similar edits (inserts or deletes) on the same line.
//!
//!
