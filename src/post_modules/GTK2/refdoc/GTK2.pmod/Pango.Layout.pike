//! Pango Layout.
//!
//!

Pango.Layout context_changed( );
//! Forces recomputation of any state in the layout that might depend on the
//! layout's context.  This function should be called if you make changes
//! to the context subsequent to creating the layout.
//!
//!

GTK2.Pango.Layout copy( GTK2.Pango.Layout src );
//! Does a copy of the src.
//!
//!

static Pango.Layout create( GTK2.Pango.Context context );
//! Create a new layout with attributes initialized to default values for
//! a particular Pango.Context
//!
//!

int get_alignment( );
//! Gets the alignment.
//!
//!

int get_auto_dir( );
//! Gets whether to calculate the bidirectional base direction for the layout
//! according to the contents of the layout.
//!
//!

GTK2.Pango.Context get_context( );
//! Returns the Pango.Context.
//!
//!

array get_cursor_pos( int index );
//! Given an index within a layout, determines the positions of the strong and
//! weak cursors if the insertion point is at that index.  The position of
//! each cursor is stored as a zero-width rectangle.  The strong cursor is
//! the location where characters of the directionality equal to the base
//! direction of the layout are inserted.  The weak cursor location is the
//! location where characters of the directionality opposite to the base
//! direction of the layout are inserted.
//! Returns:
//! array of mappings, each mapping is the same as index_to_pos().
//!
//!

int get_indent( );
//! Gets the paragraph indent in pango units.  A negative value indicates a
//! hanging indent.
//!
//!

int get_justify( );
//! Gets whether or not each complete line should be stretched to fill the
//! entire width of the layout.
//!
//!

GTK2.Pango.LayoutLine get_line( int line );
//! Retrieves a specific line.
//!
//!

int get_line_count( );
//! Retrieves the count of lines.
//!
//!

int get_single_paragraph_mode( );
//! Gets the value set by set_single_paragraph_mode().
//!
//!

mapping get_size( );
//! Determines the logical width and height in Pango units.
//!
//!

int get_spacing( );
//! Gets the amount of spacing between the lines.
//!
//!

GTK2.Pango.TabArray get_tabs( );
//! Gets the current W(TabArray) used by this layout.  If no W(TabArray) has
//! been set, then the default tabs are in use and 0 is returned.  Default
//! tabs are every 8 spaces.
//!
//!

string get_text( );
//! Gets the text.
//!
//!

int get_width( );
//! Gets the line wrap width.
//!
//!

int get_wrap( );
//! Gets the wrap mode for the layout.
//!
//!

mapping index_to_pos( int index );
//! Converts from an index to the onscreen position corresponding to the
//! grapheme at that index, which is represented as a rectangle.  Note that
//! x is always the leading edge of the grapheme and x+width the trailing
//! edge of the grapheme.  If the direction of the grapheme is right-to-left,
//! then width will be negative.
//! 
//! Returns:
//! ([ "x": x coordinate, "y": y coordinate,
//!    "width": width of the rectangle, "height": height of the rectangle ])
//!
//!

mapping move_cursor_visually( int strong, int old_index, int old_trailing, int dir );
//! Computes a new cursor position from an old position and a count of
//! positions to move visually.  If count is positive, then the new strong
//! cursor position will be one position to the right of the old cursor
//! position.  If count is negative, then the new strong cursor position will
//! be one position to the left of the old cursor position.
//! 
//! In the presence of bidirectional text, the correspondence between logical
//! and visual order will depend on the direction of the current run, and there
//! may be jumps when the cursor is moved off the end of a run.
//! 
//! Motion here is in cursor positions, not in characters, so a single call to
//! move_cursor_visually() may move the cursor over multiple characters when
//! multiple characters combine to form a single grapheme.
//!
//!

Pango.Layout set_alignment( int alignment );
//! Sets the alignment for the layout (how partial lines are positioned within
//! the horizontal space available.)  One of .
//!
//!

Pango.Layout set_auto_dir( int auto_dir );
//! Sets whether to calculate the bidirectional base direction for the layout
//! according to the contents of the layout; when this flag is on (the
//! default), then paragraphs that begin with strong right-to-left characters
//! (Arabic and Hebrew principally), will have right-left-layout, paragraphs
//! with letters from other scripts will have left-to-right layout.
//! Paragraphs with only neutral characters get their direction from the
//! surrounding paragraphs.
//! 
//! When false, the choice between left-to-right and right-to-left layout is
//! done by according to the base direction of the Pango.Context.
//!
//!

Pango.Layout set_font_description( GTK2.Pango.FontDescription desc );
//! Sets the default font description for the layout.  If no font
//! description is set on the layout, the font descriptions from the layout's
//! context is used.
//!
//!

Pango.Layout set_indent( int indent );
//! Sets the width in pango units to indent each paragraph.  A negative value
//! of indent will produce a hanging indent.  That is, the first line will
//! have the full width, and subsequent lines will be indented by the absolute
//! value of indent.
//!
//!

Pango.Layout set_justify( int justify );
//! Sets whether or not each complete line should be stretched to fill the
//! entire width of the layout.  This stretching is typically done by adding
//! whitespace, but for some scripts (such as Arabic), the justification is
//! done by extending the characters.
//!
//!

Pango.Layout set_markup( string markup, int length );
//! Same as set_markup_with_accel(), but the markup text isn't scanned for
//! accelerators.
//!
//!

Pango.Layout set_markup_with_accel( string markup, int length, int|void accel_marker );
//! Sets the layout text and attribute from marked-up text.  Replaces the
//! current text and attribute list.
//! 
//! If accel_marker is nonzero, the given character will mark the character
//! following it as an accelerator.  For example, the accel marker might be an
//! ampersand or underscore.  All characters marked as an acclerator will
//! receive a GTK2.PANGO_UNDERLINE_LOW attribute, and the first character so
//! marked will be returned.  Two accel_marker characters following each other
//! produce a single literal accel_marker character.
//!
//!

Pango.Layout set_single_paragraph_mode( int setting );
//! If setting is true, do not treat newlines and similar characters as
//! paragraph separators; instead, keep all text in a single paragraph, and
//! display a glyph for paragraph separator characters.  Used when you want to
//! allow editing of newlines on a single text line.
//!
//!

Pango.Layout set_spacing( int spacing );
//! Sets the amount of spacing between the lines.
//!
//!

Pango.Layout set_tabs( GTK2.Pango.TabArray tabs );
//! Sets the tabs to use, overriding the default tabs (by default, tabs are
//! every 8 spaces).  If tabs is omitted, the default tabs are reinstated.
//!
//!

Pango.Layout set_text( string text, int length );
//! Sets the text of the layout.
//!
//!

Pango.Layout set_width( int width );
//! Sets the width to which the lines should be wrapped.
//!
//!

Pango.Layout set_wrap( int wrap );
//! Sets the wrap mode; the wrap mode only has an effect if a width is set on
//! the layout with set_width().  To turn off wrapping, set the width to -1.
//! One of .
//!
//!

mapping xy_to_index( int x, int y );
//! Converts from x and y position within a layout to the byte index to the
//! character at that logical position.  If the y position is not inside the
//! layout, the closest position is chosen (the position will be clamped inside
//! the layout).  If the X position is not within the layout, then the start
//! or the end of the line is chosen as describe for x_to_index().  If either
//! the x or y positions were not inside the layout, then returns 0.
//! 
//! Returns:
//! ([ "index": byte index, "trailing": where in grapheme user clicked ]).
//!
//!
