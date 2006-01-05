//! Properties:
//! int auto-indent
//! int highlight-current-line
//! int insert-spaces-instead-of-tabs
//! int margin
//! int show-line-markers
//! int show-line-numbers
//! int show-margin
//! int smart-home-end
//! int tabs-width
//!
//!
//!  Signals:
//! @b{redo@}
//!
//! @b{undo@}
//!

inherit GTK2.TextView;

static GTK2.SourceView create( GTK2.SourceBuffer buffer );
//! Create a new W(SourceView).  If a buffer isn't specified, an empty default
//! buffer will be created.
//!
//!

int get_auto_indent( );
//! Returns whether auto indentation of text is enabled.
//!
//!

GTK2.SourceStyleScheme get_default( );
//! Gets the default style scheme.
//!
//!

int get_insert_spaces_instead_of_tabs( );
//! Returns whether when inserting a tabulator character it should be replaced
//! by a group of space characters.
//!
//!

int get_margin( );
//! Gets the position of the right margin.
//!
//!

GTK2.GdkPixbuf get_marker_pixbuf( string type );
//! Gets the pixbuf which is associated with the given type.
//!
//!

int get_show_line_numbers( );
//! Returns whether line markers are displayed beside the text.
//!
//!

int get_show_margin( );
//! Returns whether a margin is displayed.
//!
//!

int get_smart_home_end( );
//! Returns whether HOME and END keys will move to the first/last non-space
//! character of the line before moving to the start/end.
//!
//!

int get_tabs_width( );
//! Returns the width of tabulation in characters.
//!
//!

GTK2.SourceView set_auto_indent( int setting );
//! If true, auto indentation of text is enabled.
//!
//!

GTK2.SourceView set_insert_spaces_instead_of_tabs( int setting );
//! If true, any tabulator character inserted is replaced by a group of space
//! characters.
//!
//!

GTK2.SourceView set_margin( int setting );
//! Sets the position of the right margin in the given view.
//!
//!

GTK2.SourceView set_marker_pixbuf( string type, GTK2.GdkPixbuf pixbuf );
//! Associates a given pixbuf with a given marker type.
//!
//!

GTK2.SourceView set_show_line_markers( int setting );
//! If true, line makers will be displayed beside the text.
//!
//!

GTK2.SourceView set_show_line_numbers( int setting );
//! If true, line numbers will be displayed beside the text.
//!
//!

GTK2.SourceView set_show_margin( int setting );
//! If true, a margin is displayed.
//!
//!

GTK2.SourceView set_smart_home_end( int setting );
//! If true, HOME and END keys will move to the first/last non-space character
//! of the line before moving to the start/end.
//!
//!

GTK2.SourceView set_tabs_width( int width );
//! Sets the width of tabulation in characters.
//!
//!
