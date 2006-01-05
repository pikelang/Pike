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

static GTK2.SourceBuffer create( GTK2.SourceTagTable table_or_lang );
//! Creates a new source buffer.  If table_or_lang is a GTK2.SourceLanguage,
//! it will create a buffer using the highlighting patterns of that language.
//! If it is a GTK2.SourceTagTable, it will use that table, otherwise it will
//! be a new source buffer with a new tag table.
//!
//!

GTK2.SourceMarker create_marker( string name, string type, GTK2.TextIter where );
//! Creates a marker in the buffer of type type.  A marker is semantically very
//! similar to a GTK2.TextMark, except it has a type which is used by the
//! GTK2.SourceView displaying the buffer to show a pixmap on the left margin,
//! at the line the marker is in.  Becuase of this, a marker is generally
//! associated to a line and not a character position.  Markers are also
//! accessible through a position or range in the buffer.
//! 
//! Markers are implemented using GTK2.TextMark, so all characteristics and
//! restrictions to marks apply to markers too.  These include life cycle
//! issues and "mark-set" and "mark-deleted" signal emissions.
//! 
//! Like a GTK2.TextMark, a GTK2.SourceMarker can be anonymous if the passed
//! name is 0.
//! 
//! Markers always have left gravity and are moved to the beginning of the line
//! when the users deletes the line they were in.  Also, if the user deletes a
//! region of text which contained lines with markers, those are deleted.
//! 
//! Typical uses for a marker are bookmarks, breakpoints, current executing
//! instruction indication in a source file, etc.
//!
//!

GTK2.SourceBuffer delete_marker( GTK2.SourceMarker marker );
//! Deletes marker from the source buffer.  The same conditions as for
//! GTK2.TextMark apply here.
//!
//!

GTK2.SourceBuffer end_not_undoable_action( );
//! Marks the end of a not undoable action on the buffer.  When the last not
//! undoable block is closed through a call to this function, the list of undo
//! actions is cleared and the undo manager is re-enabled.
//!
//!

int get_check_brackets( );
//! Determines whether bracket match highlighting is activated.
//!
//!

int get_escape_char( );
//! Determines the escaping character used by the source buffer highlighting
//! engine.
//!
//!

GTK2.SourceMarker get_first_marker( );
//! Returns the first (nearest to the top of the buffer) marker.
//!
//!

int get_highlight( );
//! Determines whether text highlighting is activated in the source buffer.
//!
//!

GTK2.TextIter get_iter_at_marker( GTK2.SourceMarker marker );
//! Returns a GTK2.TextIter at marker.
//!
//!

GTK2.SourceLanguage get_language( );
//! Determines the GTK2.SourceLanguage used by the buffer.
//!
//!

GTK2.SourceMarker get_last_marker( );
//! Returns the last (nearest to the bottom of the buffer) marker.
//!
//!

GTK2.SourceMarker get_marker( string name );
//! Looks up the GTK2.SourceMarker named name, or returns 0 if it doesn't exist.
//!
//!

array get_markers_in_region( GTK2.TextIter begin, GTK2.TextIter end );
//! Returns an ordered (by position) list of GTK2.SourceMarker objects inside
//! the region delimited by the GTK2.TextIters begin and end.  The iters may
//! be in any order.
//!
//!

int get_max_undo_levels( );
//! Determines the number of undo levels the buffer will track for buffer edits.
//!
//!

GTK2.SourceMarker get_next_marker( GTK2.TextIter iter );
//! Returns the nearest marker to the right of iter.  If there are multiple
//! markers at the same position, this function will always return the first
//! one (from the internal linked list), even if starting the search exactly
//! at its location.  You can get the others using next().
//!
//!

GTK2.SourceMarker get_prev_marker( GTK2.TextIter iter );
//! Returns the nearest marker to the left of iter.  If there are multiple
//! markers at the same position, this function will always return the last one
//! (from the internal linked list), even if starting the search exactly at
//! its location.  You can get the others using prev().
//!
//!

GTK2.SourceBuffer move_marker( GTK2.SourceMarker marker, GTK2.TextIter where );
//! Moves marker to the new location.
//!
//!

GTK2.SourceBuffer redo( );
//! Redoes the last undo operation.  Use can_redo() to check whether a call to
//! this function will have any effect.
//!
//!

GTK2.SourceBuffer set_bracket_match_style( mapping style );
//! Sets the style used for highlighting matching brackets.
//! <code>
//! ([ "default": boolean,
//!    "mask": int,
//!    "foreground": GDK2.Color,
//!    "background": GDK2.Color,
//!    "italic": boolean,
//!    "bold": boolean,
//!    "underline": boolean,
//!    "strikethrough": boolean
//! ]);
//! </code>
//!
//!

GTK2.SourceBuffer set_check_brackets( int setting );
//! Controls the bracket match highlighting function in the buffer.  If
//! activated, when you position your cursor over a bracket character (a
//! parenthesis, a square bracket, etc.) the matching opening or closing
//! bracket character will be highlighted.  You can specify the style with the
//! set_bracket_match_style() function.
//!
//!

GTK2.SourceBuffer set_escape_char( int escape_char );
//! Sets the escape character to be used by the highlighting engine.
//! 
//! When performing the initial analysis, the engine will discard a matching
//! syntax pattern if it's prefixed with an odd number of escape characters.
//! This allows for example to correctly highlight strings with escaped quotes
//! embedded.
//! 
//! This setting affects only syntax patterns.
//!
//!

GTK2.SourceBuffer set_highlight( int setting );
//! Controls whether text is highlighted in the buffer.  If setting is true,
//! the text will be highlighted according to the patterns installed in the
//! buffer (either set with set_language() or by adding individual
//! GTK2.SourceTag tags to the buffer's tag table).  Otherwise, any current
//! highlighted text will be restored to the default buffer style.
//! 
//! Tags not of GTK2.SourceTag type will not be removed by this option, and
//! normal GTK2.TextTag priority settings apply when highlighting is enabled.
//! 
//! If not using a GTK2.SourceLanguage for setting the highlighting patterns in
//! the buffer, it is recommended for performance reasons that you add all the
//! GTK2.SourceTag tags with highlighting disabled and enable it when finished.
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
