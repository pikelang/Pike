//! A TextIter.
//!
//!

int backward_char( );
//! Moves backward by one character offset.  Returns true if the movement
//! was possible; if it was the first in the buffer (character offset 0),
//! backward_char() returns false for convenience when writing loops.
//!
//!

int backward_chars( int count );
//! Moves count characters backward, if possible (if count would mmove past
//! the start or end of the buffer, moves to the start or end of the buffer).
//! The return value indicates whether the iterator moved onto a
//! dereferenceable position; if it didn't move, or moved onto the end
//! iterator, then false is returned.  If count is 0, the function does
//! nothing and returns false.
//!
//!

int backward_cursor_position( );
//! Like forward_cursor_position(), but moves backward.
//!
//!

int backward_cursor_positions( int count );
//! Moves up to count cursor positions.
//!
//!

int backward_line( );
//! Moves to the start of the previous line.  Returns true if this iter
//! could be moved; i.e. if iter was at character offset 0, this function
//! returns false.  Therefore if it was already on line 0, but not at
//! the start of the line, it is snapped to the start of the line and the
//! function returns true.  (Note that this implies that in a loop calling
//! this function, the line number may not change on every iteration, if
//! your first iteration is on line 0.)
//!
//!

int backward_lines( int count );
//! Moves count lines backward.
//!
//!

array backward_search( string str, int flags, GTK2.TextIter limit );
//! Same as forward_search(), but searches backward.
//!
//!

int backward_sentence_start( );
//! Moves backward to the previous sentence start.
//!
//!

int backward_sentence_starts( int count );
//! Call backward_sentence_start() count times.
//!
//!

int backward_to_tag_toggle( GTK2.TextTag tag );
//! Moves backward to the next toggle.  See forward_to_tag_toggle().
//!
//!

int backward_visible_cursor_position( );
//! Moves backward to the previous visible cursor position.
//!
//!

int backward_visible_cursor_positions( int count );
//! Moves up to count visible cursor positions.
//!
//!

int backward_visible_line( );
//! Moves to the start of the previous visible line.  Returns TRUE if iter
//! could be moved; i.e. if iter was at character offset 0, this function
//! returns FALSE.  Therefore if iter was alreayd on line 0, but not at the
//! start of the line, iter is snapped to the start of the line and the
//! function returns TRUE. (Note that this implies that in a loop calling this
//! function, the line number may not change on every iteration, if your first
//! iteration is on line 0).
//!
//!

int backward_visible_lines( int count );
//! Moves count visible lines backward, if possible (if count would move past
//! the start or end of the buffer, moves to the start or end of the buffer).
//! The return value indicates whether the iterator moved onto a
//! dereferenceable position; if the iterator didn't move, or moved onto the
//! end iterator, then FALSE is returned.  If count is 0, the function does
//! nothing and returns FALSE.  If count is negative, moves forward by 0 -
//! count lines.
//!
//!

int backward_visible_word_start( );
//! Moves backward to the previous visible word start.
//!
//!

int backward_visible_word_starts( int count );
//! Call backward_visible_word_start() count times
//!
//!

int backward_word_start( );
//! Moves backward to the previous word start.
//!
//!

int backward_word_starts( int count );
//! Calls backward_word_start() up to count times.
//!
//!

int begins_tag( GTK2.TextTag tag );
//! Returns true if tag is toggled on at exactly this point.  If tag is
//! omitted, returns true if any tag is toggled on at this point.  Note
//! that begins_tag() returns true if this iter is the start of the tagged
//! range; has_tag() tells you whether an iterator is within a tagged range.
//!
//!

int can_insert( int default_editability );
//! Considering the default editability of the buffer, and tags that
//! affect editability, determines whether text inserted here would be
//! editabled.  If text inserted here would be editabled then the user
//! should be allowed to insert text here.  insert_interactive() uses this
//! function to decide whether insertions are allowed at a given position.
//!
//!

int compare( GTK2.TextIter rhs );
//! Returns -1 if this iterator is less than rhs, 1 if greater than,
//! and 0 if they're equal.  Ordering is in character offset order, i.e.
//! the first character in the buffer is less than the second character
//! in the buffer.
//!
//!

GTK2.TextIter destroy( );
//! Destructor.
//!
//!

int editable( int default_setting );
//! Returns whether the character at this location is within an editable
//! region of text.  Non-editable text is "locked" and can't be changed
//! by the user via W(TextView).  This function is simply a convenience
//! wrapper around get_attributes().  If no tags applied to this text
//! editability, default_setting will be returned.
//! 
//! You don't want to use this function to decide whether text can be
//! inserted here, because for insertion you don't want to know whether
//! the char at iter is inside an editable range, you want to know whether
//! a new characer inserted here would be inside an editable range.  Use
//! can_insert() to handle this case.
//!
//!

int ends_line( );
//! Returns true if iter points to the start of the paragraph delimiter
//! characters for a line (delimiters will be either a newline, a carriage
//! return, a carriage return followed by a newline, or a Unicode paragraph
//! separator character).  Note that an iterator pointing to the \n of a
//! \r\n pair will not be counted as the end of a line, the line ends before
//! the \r.  The end iterator is considered to be at the end of a line,
//! even though there are no paragraph delimiter chars there.
//!
//!

int ends_sentence( );
//! Determines whether this iter ends a sentence.
//!
//!

int ends_tag( GTK2.TextTag tag );
//! Returns true if tag is toggled off at exactly this point.  If tag is
//! omitted, returns true if any tag is toggled off at this point.  Not that
//! ends_tag() returns true if this iter it at the end of the tagged range;
//! has_tag() tells you whether an iterator is within a tagged range.
//!
//!

int ends_word( );
//! Determines whether this iter ends a natural-language word.  Word breaks
//! are determined by Pango and should be correct for nearly any language
//! (if not, the correct fix would be to the Pango word break algorithms).
//!
//!

int equal( GTK2.TextIter rhs );
//! Tests whether two iterators are equal, using the fastest possible
//! mechanism.  This function is very fast; you can expect it to perform
//! better than e.g. getting the character offset for each iterator and
//! comparing offsets yourself.  Also, it's a bit faster than compare().
//!
//!

int forward_char( );
//! Moves this iterator forward by one character offset.  Note that images
//! embedded in the buffer occopy 1 character slot, to forward_char() may
//! actually move onto an image instead of a character, if you have images
//! in your buffer.  If this iterator is the end iterator or one character
//! before it, it will now point at the end iterator, and forward_char()
//! returns false for convenience when writing loops.
//!
//!

int forward_chars( int count );
//! Moves count characters if possible (if count would move past the start or
//! end of the buffer, moves to the start or end of the buffer).  The return
//! value indicates whether the new position is different from its original
//! position, and dereferenceable (the last iterator in the buffer is not).
//! If count is 0, the function does nothing and returns false.
//!
//!

int forward_cursor_position( );
//! Moves this iterator forward by a single cursor position.  Cursor
//! positions are (unsurprisingly) positions where the cursor can appear.
//! Perhaps surprisingly, there may not be a cursor position between all
//! characters.  The most common example for European languages would be
//! a carriage return/newline sequence.  For some Unicode characters, the
//! equivalent of say the letter "a" with an accent mark will be
//! represented as two characters, first the letter then a "combining mark"
//! that causes the accent to be rendered; so the cursor can't go between
//! those two characters.  See also Pango.LogAttr and pango_break().
//!
//!

int forward_cursor_positions( int count );
//! Moves up to count cursor positions.
//!
//!

int forward_line( );
//! Moves to the start of the next line.  Returns true if there was a next
//! line to move to, and false if this iter was simply moved to the end
//! of the buffer and is now not dereferenceable, or if it was already at
//! the end of the buffer.
//!
//!

int forward_lines( int count );
//! Moves count lines forward, if possible (if count would move past the
//! start or end of the buffer, moves to the start or end of the buffer).
//! The return value indicates whether the iterator moved onto a
//! dereferenceable position; if the iterator didn't move, or moved onto the
//! end iterator, then false is returned.  If count is 0, the function does
//! nothing and returns false.  If count is negative, moves backward by
//! 0 - count lines.
//!
//!

array forward_search( string str, int flags, GTK2.TextIter limit );
//! Searches forward for str.  Returns two GTK2.TextIter objects, one
//! pointing to the first character of the match, and the second pointing
//! to the first character after the match.  The search will not continue
//! past limit.  Note that a search is a linear or O(n) operation, so you
//! may wish to use limit to avoid locking up your UI on large buffers.
//! 
//! If the GTK2.TEXT_SEARCH_VISIBLE_ONLY flag is present, the match may have
//! invisible text interspersed in str, i.e. str will be a possibly
//! non-contiguous subsequence of the matched range.  Similarly, if you
//! specify GTK2.TEXT_SEARCH_TEXT_ONLY, the match may have pixbufs or
//! child widgets mixed inside the matched range.  If these flags are not
//! given, the match must be exact; the special 0xFFFC character in str
//! will match embedded pixbufs or child widgets.
//!
//!

int forward_sentence_end( );
//! Moves forward to the next sentence end.
//!
//!

int forward_sentence_ends( int count );
//! Call forward_sentence_ends() count times.
//!
//!

GTK2.TextIter forward_to_end( );
//! Moves forward to the "end iterator", which points one past the last
//! valid character in the buffer.  get_char() called on the end iterator
//! returns 0, which is convenient for writing loops.
//!
//!

int forward_to_line_end( );
//! Moves the iterator to point to the paragraph delimiter characters, which
//! will be either a newline, a carriage return, a carriage return/newline
//! sequence, or the Unicode paragraph separator character.  If the iterator
//! is already at the paragraph delimiter characters, moves to the
//! paragraph delimiter characters for the next line.  If iter is on the
//! last line in the buffer, which does not end in paragraph delimiters,
//! moves to the end iterator (end of the last line), and returns false.
//!
//!

int forward_to_tag_toggle( GTK2.TextTag tag );
//! Moves forward to the next toggle (on or off) of tag, or to the next
//! toggle of any tag if tag is omitted.  If no matching tag toggles are
//! found, returns false, otherwise true.  Does not return toggles located
//! at this iter, only toggles after.  Sets this iter to the location of
//! the toggle, or to the end of the buffer if no toggle is found.
//!
//!

int forward_visible_cursor_position( );
//! Moves forward to the next visible cursor position.
//!
//!

int forward_visible_cursor_positions( int count );
//! Moves up to count visible cursor positions.
//!
//!

int forward_visible_line( );
//! Moves to the start of the next visible line.  Returns TRUE if there was a
//! next line to move to, and FALSE if iter was simply moved to the end of the
//! buffer and is now not dereferenceable, or if iter was already at the end
//! of the buffer.
//!
//!

int forward_visible_lines( int count );
//! Moves count visible lines forward, if possible (if count would move past
//! the start or end of the buffer, moves to the start or end of the buffer).
//! The return value indicates whether the iterator moved onto a
//! dereferenceable position; if the iterator didn't move, or moved onto the
//! end iterator, then FALSE is returned.  If count is 0, the function does
//! nothing and returns FALSE.  If count is negative, moves backward by
//! 0 - count lines.
//!
//!

int forward_visible_word_end( );
//! Moves forward to the next visible word end.
//!
//!

int forward_visible_word_ends( int count );
//! Call forward_visible_word_end() count times.
//!
//!

int forward_word_end( );
//! Moves forward to the next word end.
//!
//!

int forward_word_ends( int count );
//! Calls forward_word_end() up to count times.
//!
//!

GTK2.TextIter free( );
//! Free an iterator.
//!
//!

GTK2.TextBuffer get_buffer( );
//! Returns the W(TextBuffer) this iterator is associated with.
//!
//!

int get_bytes_in_line( );
//! Returns the number of bytes in the line, including the paragraph
//! delimiters.
//!
//!

int get_char( );
//! Returns the Unicode character at this iterator.  If the element
//! at this iterator is a non-character element, such as an image
//! embedded in the buffer, the Unicode "unknown" character 0xFFFc
//! is returned.
//!
//!

int get_chars_in_line( );
//! Returns the number of characters in the line, including the paragraph
//! delimiters.
//!
//!

int get_line( );
//! Returns the line number containing this iterator.
//!
//!

int get_line_index( );
//! Returns the byte index of the iterator, counting from the start
//! of a newline-terminated line.  Rember that W(TextBuffer) encodes
//! text in UTF-8, and that characters can require a variable number
//! of bytes to represent.
//!
//!

int get_line_offset( );
//! Returns the character offset of the iterator, counting from the
//! start of a newline-terminated line.
//!
//!

array get_marks( );
//! Returns a list of W(TextMark) at this location.  Because marks are not
//! iterable (they don't take up any "space" in the buffer, they are just
//! marks in between iterable locations), multiple marks can exist in the
//! same place.  The returned list is not in any meaningful order.
//!
//!

int get_offset( );
//! Returns the character offset of an iterator.
//!
//!

GTK2.GdkPixbuf get_pixbuf( );
//! If the element at iter is a pixbuf, it is returned.
//!
//!

string get_slice( GTK2.TextIter end );
//! Returns the text in the given range.  A "slice" is an array of
//! characters encoded in UTF-8 foramt, including the Unicode "unknown"
//! character 0xFFFC for iterable non-character elements in the buffer,
//! such as images.  Because images are encoded in the slice, byte and
//! character offsets in the returned array will correspond to bytes
//! offsets in the text buffer.  Note that 0xFFFC can occur in normal
//! text as well, so it is not a reliable indicator that a pixbuf or
//! widget is in the buffer.
//!
//!

array get_tags( );
//! Returns a list of tags that apply to iter, in ascending order of
//! priority (highest-priority tags are last).
//!
//!

string get_text( GTK2.TextIter end );
//! Returns text in the given range.  If the range contains non-text
//! elements such as images, the character and byte offsets in the
//! returned string will not correspond to character and bytes offsets
//! in the buffer.
//!
//!

array get_toggle_tags( int toggled_on );
//! Returns a list of W(TextTag) that are toggled on or off at this point.
//! (If toggled_on is true, the list contains tags that are toggled on).
//! If a tag is toggled on at this iterator, then some non-empty range
//! of characters following this iter has that tag applied to it.  If a
//! tag is toggled off, then some non-empty range following this iter does
//! not have the tag applied to it.
//!
//!

int get_visible_line_index( );
//! Returns the number of bytes from the start of the line to this iter,
//! not counting bytes that are invisible due to tags with the invisible
//! flag toggled on.
//!
//!

int get_visible_line_offset( );
//! Returns the offset in characters from the start of the line, not
//! not counting characters that are invisible due to tags with the
//! invisible tag toggled on.
//!
//!

string get_visible_slice( GTK2.TextIter end );
//! Like get_slice(), but invisible text is not included.
//!
//!

string get_visible_text( GTK2.TextIter end );
//! Like get_text(), but invisible text is not include.
//!
//!

int has_tag( GTK2.TextTag tag );
//! Returns true if this iterator is within a range tagged with tag.
//!
//!

int in_range( GTK2.TextIter start, GTK2.TextIter end );
//! Checks whether this iterator falls in the range [start,end).  start
//! and end must be in ascending order.
//!
//!

int inside_sentence( );
//! Determines whether this is inside a sentence (as opposed to int between
//! two sentences, e.g. after a period and before the first letter of the
//! next sentence).  Sentence boundaries are determined by Pango and should
//! be correct for nearly language (if not, the correct fix would be to the
//! Pango text boundary algorithms).
//!
//!

int inside_word( );
//! Determines whether this iter is inside a natural-language word
//! ass opposed to say inside some whitespace).
//!
//!

int is_cursor_position( );
//! See forward_cursor_position() or Pango.LangAttr or pango_break() for
//! details on what a cursor position is.
//!
//!

int is_end( );
//! Returns true if this location is the end iterator, i.e. one past the
//! last dereferenceable iterator in the buffer.  is_end() is the most
//! efficient way to check whether an iterator is the end iterator.
//!
//!

int is_start( );
//! Returns true if this is the first iterator in the buffer, that is if
//! it has a character offset of 0.
//!
//!

GTK2.TextIter order( GTK2.TextIter second );
//! Swaps this iter for second if second comes first in the buffer.  That is,
//! ensures that this iterator and second are in sequence.  Most text
//! buffer functions that take a range call this automatically on your
//! behalf, so there's no real reason to call it yourself in those cases.
//! There are some exceptions, such as in_range(), that expect a
//! pre-sorted range.
//!
//!

GTK2.TextIter set_line( int line_number );
//! Moves the iterator to the start of the line line_number.  If line_number
//! is negative or larger than the number of lines in the buffer, moves to
//! the start of the last line in the buffer.
//!
//!

GTK2.TextIter set_line_index( int index );
//! Same as set_line_offset(), but works with a byte index.  The given
//! byte index must be at the start of a character, it can't be in the
//! middle of a UTF-8 encoded character.
//!
//!

GTK2.TextIter set_line_offset( int offset );
//! Moves iterator within a line, to a new character (not byte) offset.  The
//! given character offset must be less than or equal to the number of
//! characters in the line; if equal, iterator moves to the start of the
//! next line.  See set_line_index() if you have a byte index rather than
//! a character offset.
//!
//!

GTK2.TextIter set_offset( int char_offset );
//! Sets to point to char_offset.  char_offset counts from the start of the
//! entire text buffer, starting with 0.
//!
//!

GTK2.TextIter set_visible_line_offset( int offset );
//! Like set_line_offset(), but the offset is in visible characters, i.e.
//! text with a tag making it invisible is not counted in the offset.
//!
//!

int starts_line( );
//! Returns true if this iter begins a paragraph, i.e. if get_line_offset()
//! would return 0.  However this function is potentially more efficient
//! than get_line_offset() because it doesn't have to computer the offset,
//! it just has to see whether it's 0.
//!
//!

int starts_sentence( );
//! Determines whether this iter begins a sentence.
//!
//!

int starts_word( );
//! Determines whether this iter begins a natural-language word.  Word
//! breaks are determined by Pango and should be correct for nearly any
//! language (if not, the correct fix would be to the Pango word break
//! algorithms).
//!
//!

int toggles_tag( GTK2.TextTag tag );
//! This is equivalent to begins_tag()||ends_tag(), i.e it tells you
//! whether a range with tag applied to it beings or ends here.
//!
//!
