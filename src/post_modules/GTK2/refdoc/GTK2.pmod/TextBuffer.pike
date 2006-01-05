//! Properties:
//! GTK2.TextTagTable tag_table
//!
//!
//!  Signals:
//! @b{apply_tag@}
//!
//! @b{begin_user_action@}
//!
//! @b{changed@}
//!
//! @b{delete_range@}
//!
//! @b{end_user_action@}
//!
//! @b{insert_child_anchor@}
//!
//! @b{insert_pixbuf@}
//!
//! @b{insert_text@}
//!
//! @b{mark_deleted@}
//!
//! @b{mark_set@}
//!
//! @b{modified_changed@}
//!
//! @b{remove_tag@}
//!

inherit G.Object;

GTK2.TextBuffer add_selection_clipboard( GTK2.Clipboard clipboard );
//! Adds clipboard to the list of clipboards in which the selection contents
//! of the buffer are available.  In most cases, clipboard will be the
//! GTK2.Clipboard of GDK2.SELECTION_PRIMARY for a view of this buffer.
//!
//!

GTK2.TextBuffer apply_tag( GTK2.TextTag tag, GTK2.TextIter start, GTK2.TextIter end );
//! Emits the "apply-tag" signal.  The default handler for the signal
//! applies tag to the given range, start and end do not have to be in order.
//!
//!

GTK2.TextBuffer apply_tag_by_name( string name, GTK2.TextIter start, GTK2.TextIter end );
//! Calls GTK2.TextTagTable->lookup() on the buffers tag table to get a
//! GTK2.TextTag, then calls apply_tag().
//!
//!

int backspace( GTK2.TextIter iter, int interactive, int default_editable );
//! Performs the appropriate action as if the user hit the delete key with
//! the cursor at the position specified by iter.  In the normal case a
//! single character will be deleted, but when combining accents are
//! involved, more than one character can be deleted, and when precomposed
//! character and accent combinations are involved, less than one character
//! will be deleted.
//! 
//! Because the buffer is modified, all outstanding iterators become invalid
//! after calling this function;  however, iter will be re-initialized to
//! point to the location where text was deleted.
//!
//!

GTK2.TextBuffer begin_user_action( );
//! Called to indicate that the buffer operations between here and call
//! end_user_action() are part of a single user-visible operation.  The
//! operations between begin_user_action() and end_user_action() can then be
//! grouped when creating an undo stack.  W(TextBuffer) maintains a count of
//! calls to begin_user_action() that have not been closed with a call to
//! end_user_action(), and emits the "begin-user-action" and
//! "end-user-action" signals only for the outermost pair of calls.  This
//! allows you to build user actions from other user actions.
//! 
//! The "interactive" buffer mutation functions, such as insert_interactive(),
//! automatically call begin/end user action around the buffer operations
//! they perform, so there's no need to add extra calls if your user action
//! consists solely of a single call to one of those functions.
//!
//!

GTK2.TextBuffer copy_clipboard( GTK2.Clipboard clipboard );
//! Copies the currently-selected text to a clipboard.
//!
//!

static GTK2.TextBuffer create( GTK2.TextTagTable table_or_props );
//! Creates a new text buffer.
//!
//!

GTK2.TextChildAnchor create_child_anchor( GTK2.TextIter iter );
//! This is a convenience function which simply creates a child anchor with
//! GTK2.TextChildAnchor->create() and inserts it into the buffer with
//! insert_child_anchor().  The new anchor is owned by the buffer; no
//! reference count is returned.
//!
//!

GTK2.TextMark create_mark( GTK2.TextIter where, int left_gravity, string|void mark_name );
//! Creates a mark at position where.  If mark_name is omitted, the mark is
//! anonymous; otherwise, the mark can be retrieve by name using get_mark().
//!  If a mark has left gravity, and text is inserted at the mark's current
//! location, the mark will be moved to the left of the newly-inserted text.
//! If the mark has right gravity, the mark will end up on the right of the
//! newly-inserted text.  The standard left-to-right cursor is a mark with
//! right gravity (when you type, the cursor stays on the right side of the
//! text you're typing).
//! Emits the "mark-set" signal as notification of the mark's initial
//! placement.
//!
//!

GTK2.TextTag create_tag( string tag_name, mapping props );
//! Creates a tag and adds it to the tag table.  Equivalent to calling
//! GTK2.TextTag->create() and then adding the tag to the tag table.  The
//! returned tag is owned by the tag table.
//!
//!

GTK2.TextBuffer cut_clipboard( GTK2.Clipboard clipboard, int default_editable );
//! Copies the currently-selected text to a clipboard, then deletes said
//! text if it's editable.
//!
//!

GTK2.TextBuffer delete( GTK2.TextIter start, GTK2.TextIter end );
//! Deletes text between start and end.  The order of start and end is not
//! actually relevant; delete() will reorder them.  This function actually
//! emits the "delete-range" signal, and the default handler of that signal
//! deletes the text.  Because the buffer is modified, all outstanding
//! iterators become invalid after calling this function; however, start
//! and end will be re-initialized to point to the location where text was
//! deleted.
//!
//!

int delete_interactive( GTK2.TextIter start, GTK2.TextIter end, int default_editable );
//! Deletes all editable text in the given range.  Calls delete() for each
//! editable sub-range of [start,end).  start and end are revalidated to
//! point to the location of the last deleted range, or left untouched if no
//! text was deleted.
//!
//!

GTK2.TextBuffer delete_mark( GTK2.TextMark mark );
//! Deletes mark, so that it's no longer located anywhere in the buffer.
//! Removes the reference the buffer holds to the mark.  There is no way
//! to undelete a mark.
//! The "mark-deleted" signal will be emitted as notification after the mark
//! is deleted.
//!
//!

GTK2.TextBuffer delete_mark_by_name( string name );
//! Deletes the mark named name; the mark must exist.
//!
//!

int delete_selection( int interactive, int default_editable );
//! Deletes the range between the "insert" and "selection_bound" marks, that
//! is, the currently-selected text.  If interactive is true, the editability
//! of the selection will be considered (users can't delete uneditable text).
//!
//!

GTK2.TextBuffer end_user_action( );
//! Should be paired with begin_user_action();
//!
//!

array get_bounds( );
//! Retrieves the first and last iterators in the buffer, i.e. the entire
//! buffer lies within the range [start,end).
//!
//!

int get_char_count( );
//! Gets the number of characters in the buffer; note that characters
//! and bytes are not the same, you can't e.g. expect the contents of
//! the buffer in string form to be this many bytes long.
//!
//!

GTK2.TextIter get_end_iter( );
//! Returns the "end iterator", one past the last valid character in the
//! buffer.  If dereferenced with W(TextIter)->get_char(), the end iterator
//! has a character value of 0.  The entire buffer lies in the range from
//! the first position in the buffer to the end iterator.
//!
//!

GTK2.TextMark get_insert( );
//! Returns the mark that represents the cursor (insertion point).
//! Equivalent to calling get_mark() to get the mark named "insert", but very
//! slightly more efficient, and involves less typing.
//!
//!

GTK2.TextIter get_iter_at_child_anchor( GTK2.TextChildAnchor anchor );
//! Returns the location of anchor.
//!
//!

GTK2.TextIter get_iter_at_line( int line );
//! Returns a W(TextIter) to the start of the given line.
//!
//!

GTK2.TextIter get_iter_at_line_index( int line, int byte_index );
//! Obtains an iterator point to byte_index with the given line.  byte_index
//! must be the start of a UTF-8 character, and must not be beyond the end
//! of the line.  Note bytes, not characters; UTF-8 may encode one character
//! as multiple bytes.
//!
//!

GTK2.TextIter get_iter_at_line_offset( int line_number, int char_offset );
//! Obtains an iterator pointing to char_offset within the given line.  The
//! char_offset must exist, offsets off the end of the line are not allowed.
//! Note characters, not bytes;  UTF-8 may encode one character as multiple
//! bytes.
//!
//!

GTK2.TextIter get_iter_at_mark( GTK2.TextMark mark );
//! Returns an iterator with the current position of mark.
//!
//!

GTK2.TextIter get_iter_at_offset( int offset );
//! Returns an iterator at position offset chars from the start of the
//! entire buffer.  If offset is -1 or greater than the number of characters
//! in the buffer, returns the end iterator, the iterator one past the last
//! valid character in the buffer.
//!
//!

int get_line_count( );
//! Obtains the number of lines in the buffer.
//!
//!

GTK2.TextMark get_mark( string name );
//! Returns the mark named name.
//!
//!

int get_modified( );
//! Indicates whether the buffer has been modified since the last call to
//! set_modified() set the modification flag to false.  Used for example to
//! enable a "save" function in a text editor.
//!
//!

GTK2.TextMark get_selection_bound( );
//! Returns the mark that represents the selection bound.  Equivalent to
//! calling get_mark() to get the mark named "selection_bound", but very
//! slightly more efficient, and involves less typing.
//! 
//! The currently-selected text in the buffer is the region between the
//! "selection_bound" and "insert" marks.  If "selection_bound" and "insert"
//! are in the same place, then there is no current selection.
//! get_selection_bounds() is another convenient function for handling the
//! selection, if you just want to know whether there's a selection and
//! what its bounds are.
//!
//!

array get_selection_bounds( );
//! Returns either a start and end W(TextIter) if some text is selected, or
//! 2 0's.
//!
//!

string get_slice( GTK2.TextIter start, GTK2.TextIter end, int include_hidden_chars );
//! Returns the text in the range [start,end).  Excludes undisplayed text
//! (text marked with tags that set the invisibility attribute) if
//! include_hidden_chars is false.  The returned string includes a 0xFFFC
//! character whenever the buffer contains embedded images, so byte and
//! character indexes into the returned string do correspond to byte and
//! character indexes into the buffer.  Contrast with get_text().  Note that
//! 0xFFFC can occur in normal text as well, so it is not a reliable
//! indicator that a pixbuf or widget is in the buffer.
//!
//!

GTK2.TextIter get_start_iter( );
//! Returns an iterator with the first position in the text buffer.  This is
//! the same as using get_iter_at_offset() to get the iter at character
//! offset 0.
//!
//!

GTK2.TextTagTable get_tag_table( );
//! Get the W(TextTagTable) associated with this buffer.
//!
//!

string get_text( GTK2.TextIter start, GTK2.TextIter end, int include_hidden_chars );
//! Returns the text int the range [start,end).  Excludes undisplayed text
//! (text marked with tags that set the invisibility attribute) if
//! include_hidden_chars is false.  Does not include characters representing
//! embedded images, so byte and character indexes into the returned
//! string do not correspond to byte and character indexes into the buffer.
//! Contrast with get_slice().
//!
//!

GTK2.TextBuffer insert( GTK2.TextIter iter, string text, int len );
//! Insert len bytes of text at position iter.  If len is -1, string
//! will be inserted in its entirely.  Emits the 'insert-text' signal.
//! iter is invalidated after insertion, but the default signal handler
//! revalidates it to point to the end of the inserted text.
//!
//!

GTK2.TextBuffer insert_at_cursor( string text, int len );
//! Simply calls insert(), using the current cursor position as the
//! insertion point.
//!
//!

GTK2.TextBuffer insert_child_anchor( GTK2.TextIter iter, GTK2.TextChildAnchor anchor );
//! Inserts a child widget anchor into the buffer at iter.  The anchor will
//! be counted as one character in character counts, and when obtaining
//! the buffer contents as a string, will be represented by the Unicode
//! "object replacement character" oxFFFC.  Note that the "slice" variants
//! for obtaining portions of the buffer as a string include this character
//! for child anchors, but the "text" variants do not.  e.g. see get_slice()
//! and get_text().  Consider create_child_anchor() as a more convenient
//! alternative to this function.  The buffer will add a reference to the
//! anchor, so you can unref it after insertion.
//!
//!

int insert_interactive( GTK2.TextIter iter, string text, int len, int default_editable );
//! Like insert(), but the insertion will not occur if iter is at a non-
//! editable location in the buffer.  Usually you want to prevent insertions
//! at ineditable locations if the insertion results from a user action
//! (is interactive).
//! 
//! default_edtibale indicates the editability of text that doesn't have a
//! tag affecting editability applied to it.  Typically the result of
//! get_editable() is appropriate here.
//!
//!

int insert_interactive_at_cursor( string text, int len, int default_editable );
//! Calls insert_interactive() at the cursor position.
//!
//!

GTK2.TextBuffer insert_pixbuf( GTK2.TextIter iter, GTK2.GdkPixbuf pixbuf );
//! Inserts an image into the text buffer at iter.  The image will be
//! counted as one character in character counts, and when obtaining the
//! contents as a string, will be represented by the Unicode
//! "object replacement character" 0xFFFC.  Note that the "slice" variants
//! for obtaining portions of the buffer as a string include this character
//! for pixbufs, but the "text" variants do not.  e.g. see get_slice()
//! and get_text().
//!
//!

GTK2.TextBuffer insert_range( GTK2.TextIter iter, GTK2.TextIter start, GTK2.TextIter end );
//! Copies text, tags, and pixbufs between start and end (the order of
//! start and end doesn't matter) and inserts the copy at iter.  Used
//! instead of simply getting/inserting text because it preserves images
//! and tags.  If start and end are in a different buffer from this buffer,
//! the two buffers must share the same tag table.
//! 
//! Implemented via emissions of the insert-text and apply-tag signals, so
//! expect those.
//!
//!

int insert_range_interactive( GTK2.TextIter iter, GTK2.TextIter start, GTK2.TextIter end, int default_editable );
//! Same as insert_range(), but does nothing if the insertion point isn't
//! editable.  The default_editable parameter indicates whether the text is
//! editable at iter if no tags enclosing iter affect editability.
//!
//!

GTK2.TextBuffer insert_with_tags( GTK2.TextIter iter, string text, int len, array tags );
//! Inserts text into the buffer at iter, applying the list of tags to the
//! newly-inserted text.  Equivalent to calling insert(), then apply_tag() on
//! the insert text; insert_with_tags() is just a convenience function.
//!
//!

GTK2.TextBuffer insert_with_tags_by_name( GTK2.TextIter iter, string text, int len, array tag_names );
//! Same as insert_with_tags(), but allows you to pass in tag names instead of
//! tag objects.
//!
//!

GTK2.TextBuffer move_mark( GTK2.TextMark mark, GTK2.TextIter where );
//! Moves mark to the new location where.  Emits the "mark-set" signal as
//! notification of the move.
//!
//!

GTK2.TextBuffer move_mark_by_name( string name, GTK2.TextIter where );
//! Moves the mark named name (which must exist) to location where.
//!
//!

GTK2.TextBuffer paste_clipboard( GTK2.Clipboard clipboard, int|void default_editable, GTK2.TextIter location );
//! Pastes the contents of a clipboard at the insertion point, or at
//! override_location.
//!
//!

GTK2.TextBuffer place_cursor( GTK2.TextIter where );
//! This function moves the "insert" and "selection_bound" marks
//! simultaneously.  If you move them to the same place in two steps with
//! move_mark(), you will temporarily select a region in between their old
//! and new locations, which can be pretty inefficient since the 
//! temporarily-selected region will force stuff to be recalculated.  This
//! function moves them as a unit, which can be optimized.
//!
//!

GTK2.TextBuffer remove_all_tags( GTK2.TextIter start, GTK2.TextIter end );
//! Removes all tags in the range between start and end.  Be careful with
//! this function; it could remove tags added in code unrelated to the code
//! you're currently writing.  That is, using this function is probably a
//! bad idea if you have two or more unrelated code sections that add tags.
//!
//!

GTK2.TextBuffer remove_selection_clipboard( GTK2.Clipboard clipboard );
//! Removes a clipboard that was added with add_selection_clipboard().
//!
//!

GTK2.TextBuffer remove_tag( GTK2.TextTag tag, GTK2.TextIter start, GTK2.TextIter end );
//! Emits the "remove-tag" signal.  The default handler for the signal removes
//! all occurrences of tag from the given range.  start and end do not have to
//! be in order.
//!
//!

GTK2.TextBuffer remove_tag_by_name( string name, GTK2.TextIter start, GTK2.TextIter end );
//! Removes a tag.  See apply_tag_by_name().
//!
//!

GTK2.TextBuffer select_range( GTK2.TextIter ins, GTK2.TextIter bound );
//! This function removes the "insert" and "selection_bound" marks
//! simultaneously.  If you move them in two steps with move_mark(), you will
//! temporarily select a region in between their old and new locations, which
//! can be pretty inefficient since the temporarily-selected region will
//! force stuff to be recalculated.  This function moves them as a unit,
//! which can be optimized.
//!
//!

GTK2.TextBuffer set_modified( int setting );
//! Used to keep track of whether the buffer has been modified since the last
//! time it was saved.  Whenever the buffer is saved to disk, call
//! set_modified(0).  When the buffer is modified, it will automatically
//! toggle on the modified bit again.  When the modifed bit flips, the
//! buffer emits a "modified-changed" signal.
//!
//!

GTK2.TextBuffer set_text( string text, int len );
//! Deletes current contents of this buffer, and inserts text instead.  If
//! len is -1, text must be nul-terminated.  text must be valid UTF-8.
//!
//!
