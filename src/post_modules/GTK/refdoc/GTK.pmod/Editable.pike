//! The GTK.Editable class is a base class for widgets for editing text,
//! such as W(Entry) and W(Text). It cannot be instantiated by
//! itself. The editable class contains functions for generically
//! manipulating an editable widget, a large number of action signals
//! used for key bindings, and several signals that an application can
//! connect to to modify the behavior of a widget.
//!
//!
//!  Signals:
//! @b{activate@}
//! Indicates that the user has activated the widget in some
//! fashion. Generally, this will be done with a keystroke. (The
//! default binding for this action is Return for GTK.Entry and
//! Control-Return for GTK.Text.
//!
//!
//! @b{changed@}
//! Called when the text in the edit area is changed
//!
//!
//! @b{copy_clipboard@}
//! An action signal. Causes the characters in the current selection to
//! be copied to the clipboard.
//!
//!
//! @b{cut_clipboard@}
//! An action signal. Causes the characters in the current selection to
//! be copied to the clipboard and then deleted from the widget.
//!
//!
//! @b{delete_text@}
//! This signal is emitted when text is deleted from the widget by the
//! user. The default handler for this signal will normally be
//! responsible for inserting the text, so by connecting to this signal
//! and then stopping the signal with signal_emit_stop(), it is
//! possible to modify the inserted text, or prevent it from being
//! inserted entirely. The start_pos and end_pos parameters are
//! interpreted as for delete_text()
//!
//!
//! @b{insert_text@}
//! This signal is emitted when text is inserted into the widget by the
//! user. The default handler for this signal will normally be
//! responsible for inserting the text, so by connecting to this signal
//! and then stopping the signal with signal_emit_stop(), it is
//! possible to modify the inserted text, or prevent it from being
//! inserted entirely.
//!
//!
//! @b{kill_char@}
//! An action signal. Delete a single character.
//!
//!
//! @b{kill_line@}
//! An action signal. Delete a single line.
//!
//!
//! @b{kill_word@}
//! n action signal. Delete a single word.
//!
//!
//! @b{move_cursor@}
//! An action signal. Move the cursor position.
//!
//!
//! @b{move_page@}
//! An action signal. Move the cursor by pages.
//!
//!
//! @b{move_to_column@}
//! An action signal. Move the cursor to the given column.
//!
//!
//! @b{move_to_row@}
//! An action signal. Move the cursor to the given row.
//!
//!
//! @b{move_word@}
//! An action signal. Move the cursor by words.
//!
//!
//! @b{paste_clipboard@}
//! An action signal. Causes the contents of the clipboard to be pasted
//! into the editable widget at the current cursor position.
//!
//!
//! @b{set_editable@}
//! Determines if the user can edit the text in the editable widget or
//! not. This is meant to be overriden by child classes and should not
//! generally be useful to applications.
//!
//!
inherit Widget;

Editable copy_clipboard( )
//! Causes the characters in the current selection to be copied to the
//! clipboard.
//!
//!

Editable cut_clipboard( )
//! Causes the characters in the current selection to be copied to the
//! clipboard and then deleted from the widget.
//!
//!

Editable delete_selection( )
//! Deletes the current contents of the widgets selection and disclaims
//! the selection.
//!
//!

Editable delete_text( int start_pos, int end_pos )
//! Delete a sequence of characters. The characters that are deleted
//! are those characters at positions from start_pos up to, but not
//! including end_pos. If end_pos is negative, then the the characters
//! deleted will be those characters from start_pos to the end of the
//! text.
//!
//!

string get_chars( int start_pos, int end_pos )
//! Retrieves a sequence of characters. The characters that are
//! retrieved are those characters at positions from start_pos up to,
//! but not including end_pos. If end_pos is negative, then the the
//! characters retrieved will be those characters from start_pos to the
//! end of the text.
//!
//!

int get_position( )
//! Returns the cursor position
//!
//!

Editable insert_text( string text, int num_chars, int where )
//! Insert 'num_chars' characters from the text at the position 'where'.
//!
//!

Editable paste_clipboard( )
//! Causes the contents of the clipboard to be pasted into the given
//! widget at the current cursor position.
//!
//!

Editable select_region( int start_pos, int end_pos )
//! Selects a region of text. The characters that are selected are
//! those characters at positions from start_pos up to, but not
//! including end_pos. If end_pos is negative, then the the characters
//! selected will be those characters from start_pos to the end of the
//! text. are
//!
//!

Editable set_editable( int editablep )
//! Determines if the user can edit the text in the editable widget or
//! not.
//!
//!

Editable set_position( int pos )
//! Sets the cursor position.
//!
//!
