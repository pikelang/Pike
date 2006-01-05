//! A simple text label.
//!@expr{ GTK2.Label("A simple text label")@}
//!@xml{<image>../images/gtk2_label.png</image>@}
//!
//!@expr{ GTK2.Label("Multi\nline text\nlabel here")@}
//!@xml{<image>../images/gtk2_label_2.png</image>@}
//!
//!@expr{ GTK2.Label("Multi\nline text\nlabel here")->set_justify(GTK2.JUSTIFY_LEFT)@}
//!@xml{<image>../images/gtk2_label_3.png</image>@}
//!
//!@expr{ GTK2.Label("Multi\nline text\nlabel here")->set_justify(GTK2.JUSTIFY_RIGHT)@}
//!@xml{<image>../images/gtk2_label_4.png</image>@}
//!
//! Properties:
//! float angle
//! Pango.AttrList attributes
//! int cursor-position
//! int ellipsize 
//! int justfy @[JUSTIFY_CENTER], @[JUSTIFY_FILL], @[JUSTIFY_LEFT] and @[JUSTIFY_RIGHT]
//! string label
//! int max-width-chars
//! int mnemonic-keyval
//! int mnemonic-widget
//! string pattern
//! int selectable
//! int single-line-mode
//! int use-markup
//! int use-underline
//! int width-chars
//! int wrap
//!
//!
//!  Signals:
//! @b{copy_clipboard@}
//!
//! @b{move_cursor@}
//!
//! @b{populate_popup@}
//!

inherit GTK2.Misc;

static GTK2.Label create( string|mapping text_or_props );
//! Creates a new label.
//!
//!

float get_angle( );
//! Gets the angle of rotation for the label.
//!
//!

int get_ellipsize( );
//! Returns the ellipsizing position of the label.
//!
//!

int get_justify( );
//! Returns the justification of the label.
//!
//!

string get_label( );
//! Fetches the text from a label widget including any underlines indicating
//! mnemonics and Pango markup.
//!
//!

GTK2.Pango.Layout get_layout( );
//! Gets the Pango.Layout used to display the label.  The layout is useful to
//! e.g. convert text positions to pixel positions, in combination with
//! get_layout_offsets().
//!
//!

mapping get_layout_offsets( );
//! Obtains the coordinates where the label will draw the Pango.Layout
//! representing the text in the label; useful to convert mouse events into
//! coordinates inside the Pango.Layout, e.g. to take some action if some part
//! of the label is clicked.  Of course, you will need to create a GTK2.EventBox
//! to receive the events, and pack the label inside it, since labels are a
//! GTK2.NO_WINDOW widget.  Remember when using the Pango.Layout functions you
//! need to convert to and from pixels using GTK2.PANGO_SCALE.
//!
//!

int get_line_wrap( );
//! Returns whether lines in the label are automatically wrapped.
//!
//!

int get_max_width_chars( );
//! Retrieves the desired maximum width, in characters.
//!
//!

int get_mnemonic_keyval( );
//! If the label has been set so that is has a mnemonic key, this function
//! returns the keyval used for the mnemonic accelerator.  If there is no
//! mnemonic set up it returns GDK_VoidSymbol.
//!
//!

GTK2.Widget get_mnemonic_widget( );
//! Retrieves the target of the mnemonic (keyboard shortcut).
//!
//!

int get_selectable( );
//! Gets the value set by set_selectable().
//!
//!

mapping get_selection_bounds( );
//! Gets the selected range of characters in the label.  If there isn't a
//! selection, returns -1 for both start and end.
//!
//!

int get_single_line_mode( );
//! Returns whether the label is in single line mode.
//!
//!

string get_text( );
//! Fetches the text from a label widget, as displayed on the screen.  This
//! does not include any embedded underlines indicating mnemonics or Pango
//! markup. (see get_label()).
//!
//!

int get_use_markup( );
//! Returns whether the label's text is interpreted as marked up with the
//! Pango text markup language.
//!
//!

int get_use_underline( );
//! Returns whether an embedded underline in the label indicates a mnemonic.
//!
//!

int get_width_chars( );
//! Retrieves the desired width, in characters.
//!
//!

GTK2.Label select_region( int start_offset, int end_offset );
//! Selects a range of characters in the label, if the label is selectable.
//! See set_selectable().  If the label is not selectable, this function has
//! no effect.  If start_offset or end_offset are -1, then the end of the
//! label will be substituted.
//!
//!

GTK2.Label set_angle( int|float angle );
//! Sets the angle of rotation for the label.  An angle of 90 reads from 
//! bottom to top, and angle of 270, from top to bottom.  The angle setting
//! for the label is igrnored if the lable is selectable, wrapped, or 
//! ellipsized.
//!
//!

GTK2.Label set_ellipsize( int mode );
//! Sets the mode used to ellipsize (add an ellipsis: "...") to the text if
//! there is not enough space to render the entire string.
//! One of .
//!
//!

GTK2.Label set_justify( int jtype );
//! Sets the alignment of the lines in the text of the label relative to each
//! other.  GTK2.JUSTIFY_LEFT is the default value when the widget is first
//! created.  If you instead want to set the alignment of the label as a
//! whole, use set_alignment() instead.  set_justify() has no efect on labels
//! containing only a single line.  One of @[JUSTIFY_CENTER], @[JUSTIFY_FILL], @[JUSTIFY_LEFT] and @[JUSTIFY_RIGHT].
//!
//!

GTK2.Label set_label( string text );
//! Sets the text of the label.  The label is interpreted as including
//! embedded underlines and/or Pango markup depending on the values of
//! use-underline and use-markup.
//!
//!

GTK2.Label set_line_wrap( int wrap );
//! Toggles line wrapping within the widget.  True makes it break lines if
//! text exceeds the widget's size.  False lets the text get cut off by the
//! edge of the widget if it exceeds the widget size.
//!
//!

GTK2.Label set_markup( string text );
//! Parses text which is marked up with the Pango text markup language,
//! setting the label's text and attribute list based on the parse results.
//!
//!

GTK2.Label set_markup_with_mnemonic( string text );
//! Parses text which is marked up with the Pango text markup language,
//! setting the label's text and attribute list based on the parse results.
//! If characters in text are preceded by an underscore, they are underline
//! indicating that they represent a keyboard accelerator called a mnemonic.
//! 
//! The mnemonic key can be used to activate another widget, chosen
//! automatically, or explicitly using set_mnemonic_widget().
//!
//!

GTK2.Label set_max_width_chars( int n_chars );
//! Sets the desired maximum width in characters to n_chars.
//!
//!

GTK2.Label set_mnemonic_widget( GTK2.Widget widget );
//! If the label has been set so that it has a mnemonic key (using i.e. 
//! set_markup_with_mnemonic(), set_text_with_mnemonic(), or the 
//! "use_underline" property) the label can be associated with a widget that
//! is the target of the mnemonic.  When the label is inside a widget (like a
//! GTK2.Button or GTK2.Notebook tab) it is automatically associated with the
//! correct widget, but sometimes (i.e. when the target is a GTK2.Entry next
//! to the label) you need to set it explicitly using this function.
//! 
//! The target widget will be accelerated by emitting "mnemonic_activate" on
//! it.  The default handler for this signal will activate the widget if there
//! are no mnemonic collisions and toggle focus between the colliding widgets
//! otherwise.
//!
//!

GTK2.Label set_pattern( string pattern_string );
//! A string with either spaces or underscores.
//! It should be of the same length as the text.
//! 
//! When a character in the text has a matching _ in the pattern, the
//! character in the label will be underlined.
//! 
//!
//!

GTK2.Label set_selectable( int setting );
//! Selectable labels allow the user to select text from the label, for copy
//! and past.
//!
//!

GTK2.Label set_single_line_mode( int mode );
//! Sets whether the label is in single line mode.
//!
//!

GTK2.Label set_text( string text );
//! Set the text in the label
//!
//!

GTK2.Label set_text_with_mnemonic( string text );
//! Sets the label's text from the string text.  If characters in text are
//! preceded by an underscore, they are underlined indicating that they
//! represent a keyboard accelerator called a mnemonic.  The mnemonic key can
//! be used to activate another widget, chose automatically, or explicitly
//! using set_mnemonic_widget().
//!
//!

GTK2.Label set_use_markup( int setting );
//! Sets whether the text of the label contains markup in Pango's text markup
//! language.
//!
//!

GTK2.Label set_use_underline( int setting );
//! If true, an underline in the text indicates the next character should be
//! used for the mnemonic accelerator key.
//!
//!

GTK2.Label set_width_chars( int n_chars );
//! Sets the desired width in characters to n_chars.
//!
//!
