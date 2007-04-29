//! Use this widget when you want the user to input a single line of text.
//!@expr{ GTK2.Entry()->set_text("Hello world")->set_editable(1)@}
//!@xml{<image>../images/gtk2_entry.png</image>@}
//!
//! Properties:
//! int activates-default
//! int cursor-position
//! int editable
//! int has-frame
//! int invisible-char
//! int max-length
//! int scroll-offset
//! int selection-bound
//! string text
//! int visibility
//! int width-chars
//! float xalign
//! 
//! Style properties:
//!
//!
//!  Signals:
//! @b{activate@}
//!
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
//! @b{paste_clipboard@}
//!
//! @b{populate_popup@}
//!
//! @b{toggle_overwrite@}
//!

inherit GTK2.Widget;

inherit GTK2.CellEditable;

inherit GTK2.Editable;

static GTK2.Entry create( int|mapping maxlen_or_props );
//! Create a new W(Entry) widget.
//!
//!

int get_activates_default( );
//! Retrieves the value set by set_activates_default().
//!
//!

float get_alignment( );
//! Gets the value set by set_alignment().
//!
//!

GTK2.EntryCompletion get_completion( );
//! Returns the completion object.
//!
//!

int get_has_frame( );
//! Gets the value set by set_has_frame().
//!
//!

int get_invisible_char( );
//! Retrieves the character displayed in place of the real characters for
//! entries with visibility set to false.
//!
//!

GTK2.Pango.Layout get_layout( );
//! Gets the Pango.Layout used to display the entry.  The layout is useful to
//! e.g. convert text positions to pixel positions, in combination with
//! get_layout_offsets().
//! 
//! Keep in mind that the layout text may contain a preedit string, so 
//! layout_index_to_text_index() and text_index_to_layout_index() are needed
//! to convert byte indices in the layout to byte indices in the entry
//! contents.
//!
//!

mapping get_layout_offsets( );
//! Obtains the position of the Pango.Layout used to render text in the
//! entry, in widget coordinates.  Useful if you want to line up the text
//! in an entry with some other text, e.g. when using the entry to implement
//! editable cells in a sheet widget.
//! 
//! Also useful to convert mouse events into coordinates inside the
//! Pango.Layout, e.g. to take some action if some part of the entry text
//! is clicked.
//! 
//! Keep in mind that the layout text may contain a preedit string, so 
//! layout_index_to_text_index() and text_index_to_layout_index() are needed
//! to convert byte indices in the layout to byte indices in the entry
//! contents.
//!
//!

int get_max_length( );
//! Retrieves the maximum allowed length of the text.
//!
//!

string get_text( );
//! Returns the contents of the entry widget.
//!
//!

int get_visibility( );
//! Retrieves whether the text is visible.
//!
//!

int get_width_chars( );
//! Gets the value set by set_width_chars().
//!
//!

int layout_index_to_text_index( int layout_index );
//! Converts from a position in the entry contents (returned by get_text())
//! to a position in the entry's Pango.Layout (returned by get_layout()),
//! with text retrieved via Pango.Layout->get_text().
//!
//!

GTK2.Entry set_activates_default( int setting );
//! If setting is true, pressing Enter will activate the default widget for
//! the window containing the entry.  This usually means that the dialog box
//! containing the entry will be closed, since the default widget is usually
//! one of the dialog buttons.
//!
//!

GTK2.Entry set_alignment( float align );
//! Sets the alignment for the ocntents of the entry.  This controls the
//! horizontal positioning of the contents when the displayed text is shorter
//! than the width of the entry.
//!
//!

GTK2.Entry set_completion( GTK2.EntryCompletion completion );
//! Sets completion to be the auxiliary completion object to use.  All further
//! configuration of the completion mechanism is done on completion using
//! the GTK2.EntryCompletion API.
//!
//!

GTK2.Entry set_has_frame( int setting );
//! Sets whether the entry has a beveled frame around it.
//!
//!

GTK2.Entry set_invisible_char( int ch );
//! Sets the character to use in place of the actual text when 
//! set_invisibility() has been called to set text visibility to false.  i.e.
//! this is the character used in "password" mode to show the user how many
//! characters have been type.  The default invisible char is an asterisk
//! ('*').  If you set the invisible char to 0, then the user will get no
//! feedback at all; there will be no text on the screen as they type.
//!
//!

GTK2.Entry set_max_length( int maxlen );
//! Sets the maximum allowed length of the contents.  If the current contents
//! are longer than the given length, then they will be truncated to fit.
//! Range is 0-65536.  0 means no maximum.
//!
//!

GTK2.Entry set_text( string text );
//! Set the text to the specified string, replacing the current contents.
//!
//!

GTK2.Entry set_visibility( int visiblep );
//! 0 indicates invisible text (password boxes, as an example)
//!
//!

GTK2.Entry set_width_chars( int n_chars );
//! changes the size request of the entry to be about the right size for
//! n_chars characters.  Note that it changes the size request, the size can
//! still be affected by how you pack the widget into containers.  If n_chars
//! is -1, the size reverts to the default entry size.
//!
//!

int text_index_to_layout_index( int text_index );
//! Opposite of layout_index_to_text_index().
//!
//!
