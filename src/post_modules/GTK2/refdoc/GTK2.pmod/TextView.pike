//! Properties:
//! int accepts-tab
//! GTK2.TextBuffer buffer
//! int cursor-visible
//! int editable
//! int indent
//! int justification
//! int left-margin
//! int overwrite
//! int pixels-above-lines
//! int pixels-below-lines
//! int pixels-inside-wrap
//! int right-margin
//! Pango.TabArray tabs
//! int wrap-mode
//! 
//! Style properties:
//! GDK2.Color error-underline-color
//!
//!
//!  Signals:
//! @b{backspace@}
//!
//! @b{copy_clipboard@}
//!
//! @b{cut_clipboard@}
//!
//! @b{delete_from_cursor@}
//!
//! @b{insert_at_cursor@}
//!
//! @b{move_cursor@}
//!
//! @b{move_focus@}
//!
//! @b{move_viewpoert@}
//!
//! @b{page_horizontally@}
//!
//! @b{paste_clipboard@}
//!
//! @b{populate_popup@}
//!
//! @b{select_all@}
//!
//! @b{set_anchor@}
//!
//! @b{set_scroll_adjustments@}
//!
//! @b{toggle_overwrite@}
//!

inherit GTK2.Container;

GTK2.TextView add_child_at_anchor( GTK2.Widget child, GTK2.TextChildAnchor anchor );
//! Adds a child widget in the text buffer, at the given anchor.
//!
//!

GTK2.TextView add_child_in_window( GTK2.Widget child, int wintype, int xpos, int ypos );
//! Adds a child at fixed coordinates in one of the text widget's windows.
//! The window must have nonzero size (see
//! GTK2.TextView->set_border_window_size()).  Note that the child coordinates
//! are given relative to the GDK2.Window in question, and that these
//! coordinates have no sane relationship to scrolling.  When placing a child
//! in GTK2.TEXT_WINDOW_WIDGET, scrolling is irrelevant, the child floats above
//! all scrollable areas.  But when placing a child in one of the scrollable
//! windows (border windows or text window), you'll need to compute the child's
//! correct position in buffer coordinates any time scrolling occurs or buffer
//! changes occur, and then call GTK2.TextView->move_child() to update the
//! child's position.  Unfortunately there's no good way to detect that
//! scrolling has occured, using the current API; a possible hack would be to
//! update all child positions when the scroll adjustments change or the text
//! buffer changes.
//!
//!

int backward_display_line( GTK2.TextIter iter );
//! See forward_display_line().
//!
//!

int backward_display_line_start( GTK2.TextIter iter );
//! Moves the iter backward to the next display line start.
//!
//!

array buffer_to_window_coords( int wintype, int buffer_x, int buffer_y );
//! Converts coordinate (buffer_x,buffer_y) to coordinates for the window
//! win, and returns the results.  wintype is one of @[TEXT_WINDOW_BOTTOM], @[TEXT_WINDOW_LEFT], @[TEXT_WINDOW_PRIVATE], @[TEXT_WINDOW_RIGHT], @[TEXT_WINDOW_TEXT], @[TEXT_WINDOW_TOP] and @[TEXT_WINDOW_WIDGET].
//!
//!

static GTK2.TextView create( GTK2.TextBuffer buffer_or_props );
//! Create a new W(TextView).  
//!
//!

int forward_display_line( GTK2.TextIter iter );
//! Moves iter forward by one display (wrapped) line.  A display line is
//! different from a paragraph.  Paragraphs are separated by newlines or
//! other paragraph separator characters.  Display lines are created by
//! line-wrapping a paragraph.  If wrapping is turned off, display lines and
//! paragraphs will be the same.  Display lines are divided differently for
//! each view, since they depend on the view's width; paragraphs are the same
//! in all view, since they depend on the contents of the W(TextBuffer).
//!
//!

int forward_display_line_end( GTK2.TextIter iter );
//! Moves the iter forward to the next display line end.
//!
//!

int get_accepts_tab( );
//! Returns whether pressing the Tab key inserts a tab character.
//!
//!

int get_border_window_size( int wintype );
//! Gets the width of the specified border window.
//!
//!

GTK2.TextBuffer get_buffer( );
//! Returns the buffer displayed by this view.
//!
//!

int get_cursor_visible( );
//! Find out whether the cursor is being displayed.
//!
//!

GTK2.TextAttributes get_default_attributes( );
//! Obtains a copy of the default text attributes.  These are the attributes
//! used for text unless a tag overrides them.
//!
//!

int get_editable( );
//! Gets the default editability.
//!
//!

int get_indent( );
//! Gets the default indentation for paragraphs.
//!
//!

GTK2.TextIter get_iter_at_location( int x, int y );
//! Retrieves the iterator at buffer coordinates x and y.  Buffer coordinates
//! are coordinates for the entire buffer, not just the currently-displayed
//! portions.  If you have coordinates from an event, you have to convert
//! those to buffer coordinates with window_to_buffer_coords().
//!
//!

GTK2.TextIter get_iter_at_position( int x, int y );
//! Retrieves the iterator pointing to the character at buffer coordinates
//! x and y.
//!
//!

GTK2.GdkRectangle get_iter_location( GTK2.TextIter iter );
//! Gets a rectangle which roughly contains the character at iter.  The
//! rectangle position is in buffer coordinates; use buffer_to_window_coords()
//! to convert these coordinates to coordinates for one of the windows in
//! the text view.
//!
//!

int get_justification( );
//! Gets the default justification.
//!
//!

int get_left_margin( );
//! Gets the default left margin size of paragraphs.
//!
//!

GTK2.TextIter get_line_at_y( int y );
//! Returns a W(TextIter) for the start of the line containing the coordinate
//! y.  y is in buffer coordinates, convert from window coordinates with
//! window_to_buffer_coords().
//!
//!

mapping get_line_yrange( GTK2.TextIter iter );
//! Gets the y coordinate of the top of the line containing iter, and the
//! height of the line.  The coordinate is a buffer coordinate; convert to
//! window coordinates with buffer_to_window_coords().
//!
//!

int get_overwrite( );
//! Returns whether the view is in overwrite mode or not.
//!
//!

int get_pixels_above_lines( );
//! Gets the default number of pixels to put above paragraphs.
//!
//!

int get_pixels_below_lines( );
//! Gets the value set by set_pixels_below_lines().
//!
//!

int get_pixels_inside_wrap( );
//! Gets the value set by set_pixels_inside_wrap().
//!
//!

int get_right_margin( );
//! Gets the default right margin size of paragraphs.
//!
//!

GTK2.Pango.TabArray get_tabs( );
//! Gets the default tabs.  Tags in the buffer may override the defaults.
//! The return value will be 0 if "standard" (8-space) tabs are used.
//!
//!

GTK2.GdkRectangle get_visible_rect( );
//! Returns a rectangle with the currently-visible region of the buffer,
//! in buffer coordinates.  Convert to window coordinates with
//! buffer_to_window_coords().
//!
//!

GTK2.GdkWindow get_window( int wintype );
//! Retrieves the GDK2.Window corresponding to an area of the text view;
//! possible windows include the overall widget window, child windows on the
//! left, right, top, bottom, and the window that displays the text buffer.
//! Windows are 0 and nonexistent if their width or height is 0, and are
//! nonexistent before the widget has been realized.
//!
//!

int get_window_type( GTK2.GdkWindow window );
//! Usually used to find out which window an event corresponds to.  If you
//! connect to an event signal, this function should be called on
//! event->window to see which window it was.  One of @[TEXT_WINDOW_BOTTOM], @[TEXT_WINDOW_LEFT], @[TEXT_WINDOW_PRIVATE], @[TEXT_WINDOW_RIGHT], @[TEXT_WINDOW_TEXT], @[TEXT_WINDOW_TOP] and @[TEXT_WINDOW_WIDGET].
//!
//!

int get_wrap_mode( );
//! Gets the line wrapping mode.
//!
//!

GTK2.TextView move_child( GTK2.Widget child, int x, int y );
//! Updates the position of a child.
//!
//!

int move_mark_onscreen( GTK2.TextMark mark );
//! Moves a mark within the buffer so that it's located within the currently
//! visible text-area.
//!
//!

int move_visually( GTK2.TextIter iter, int count );
//! Move the iterator a given number of characters visually, treating it as
//! the strong cursor position.  If count is positive, then the new strong
//! cursor position will be count positions to the right of the old cursor
//! position.  If count is negative then the new strong cursor position will
//! be count positions to the left of the old cursor position.
//! 
//! In the presence of bidirection text, the correspondence between logical
//! and visual order will depend on the direction of the current run, and
//! there may be jumps when the cursor is moved off the end of a run.
//!
//!

int place_cursor_onscreen( );
//! Moves the cursor to the currently visible region of the buffer, if it
//! isn't there already.
//!
//!

GTK2.TextView scroll_mark_onscreen( GTK2.TextMark mark );
//! Scrolls the view the minimum distance such that mark is contained
//! within the visible area.
//!
//!

GTK2.TextView scroll_to_iter( GTK2.TextIter iter, float within_margin, int use_align, float xalign, float yalign );
//! Scrolls the view so that iter is on the screen as with scroll_to_mark().
//!
//!

GTK2.TextView scroll_to_mark( GTK2.TextMark mark, float within_margin, int use_align, float xalign, float yalign );
//! Scrolls the view so that mark is on the screen in the position indicated
//! by xalign and yalign.  An alignment of 0.0 indicates left or top, 1.0
//! indicates right or bottom, 0.5 means center.  If use_align is false,
//! the text scrolls the minimal distance to get the mark onscreen, possibly
//! not scrolling at all.  The effective screen for purposes of this function
//! is reduced by the margin of size within_margin.
//!
//!

GTK2.TextView set_accepts_tab( int accepts_tab );
//! Sets the behavior of the text widget when the Tab key is pressed.  If
//! accepts_tab is true a tab character is inserted.  If accepts_tab is false
//! the keyboard focus is moved to the next widget in the focus chain.
//!
//!

GTK2.TextView set_border_window_size( int wintype, int size );
//! Sets the width of GTK2.TEXT_WINDOW_LEFT or GTK2.TEXT_WINDOW_RIGHT, or the
//! height of GTK2.TEXT_WINDOW_TOP or GTK2.TEXT_WINDOW_BOTTOM.  Automatically
//! destroys the corresponding window if the size is set to 0, and creates
//! the window if the size is set to non-zero.  This function can only be
//! used for the "border windows", it doesn't work with GTK2.TEXT_WINDOW_WIDGET,
//! GTK2.TEXT_WINDOW_TEXT, or GTK2.TEXT_WINDOW_PRIVATE.
//!
//!

GTK2.TextView set_buffer( GTK2.TextBuffer buffer );
//! Sets buffer as the buffer being displayed.  
//!
//!

GTK2.TextView set_cursor_visible( int setting );
//! Toggles whether the insertion point is displayed.  A buffer with no
//! editable text probably shouldn't have a visible cursor, so you may want
//! to turn the cursor off.
//!
//!

GTK2.TextView set_editable( int setting );
//! Sets the default editability.  You can override this default setting with
//! tags in the buffer, using the "editable" attribute of tags.
//!
//!

GTK2.TextView set_indent( int indent );
//! Sets the default indentation for paragraphs.  May be overridden by tags.
//!
//!

GTK2.TextView set_justification( int just );
//! Sets the default justification of text.  One of @[JUSTIFY_CENTER], @[JUSTIFY_FILL], @[JUSTIFY_LEFT] and @[JUSTIFY_RIGHT].
//!
//!

GTK2.TextView set_left_margin( int margin );
//! Sets the default left margin.  May be overridden by tags.
//!
//!

GTK2.TextView set_overwrite( int overwrite );
//! Changes the overwrite mode, true for on, false for off.
//!
//!

GTK2.TextView set_pixels_above_lines( int pixels );
//! Sets the default number of blank pixels above paragraphs.  Tags in
//! the buffer may override the defaults.
//!
//!

GTK2.TextView set_pixels_below_lines( int pixels );
//! Sets the default number of blank pixels to put below paragraphs.  May be
//! overridden by tags applied to the buffer.
//!
//!

GTK2.TextView set_pixels_inside_wrap( int pixels );
//! Sets the default number of pixels of blank space to leave between
//! displayed/wrapped lines within a paragraph.  May be overridden by tags.
//!
//!

GTK2.TextView set_right_margin( int margin );
//! Sets the default right margin.  May be overridden by tags.
//!
//!

GTK2.TextView set_tabs( GTK2.Pango.TabArray tabs );
//! Sets the default tab stops for paragraphs.  Tags in the buffer may
//! override the default.
//!
//!

GTK2.TextView set_wrap_mode( int wrap_mode );
//! Sets the line wrapping.  One of @[WRAP_CHAR], @[WRAP_NONE], @[WRAP_WORD] and @[WRAP_WORD_CHAR].
//!
//!

int starts_display_line( GTK2.TextIter iter );
//! Determines whether iter is at the start of a display line.
//!
//!

array window_to_buffer_coords( int wintype, int window_x, int window_y );
//! Converts coordinates on the window identified by wintype to buffer
//! coordinates, returning the result.
//!
//!
