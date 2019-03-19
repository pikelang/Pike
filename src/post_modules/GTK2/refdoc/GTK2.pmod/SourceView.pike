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

protected GTK2.SourceView create( GTK2.SourceBuffer buffer );
//! Create a new W(SourceView).  If a buffer isn't specified, an empty default
//! buffer will be created.
//!
//!

int get_auto_indent( );
//! Returns whether auto indentation of text is enabled.
//!
//!

int get_insert_spaces_instead_of_tabs( );
//! Returns whether when inserting a tabulator character it should be replaced
//! by a group of space characters.
//!
//!

int get_show_line_numbers( );
//! Returns whether line markers are displayed beside the text.
//!
//!

int get_smart_home_end( );
//! Returns whether HOME and END keys will move to the first/last non-space
//! character of the line before moving to the start/end.
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

GTK2.SourceView set_show_line_numbers( int setting );
//! If true, line numbers will be displayed beside the text.
//!
//!

GTK2.SourceView set_smart_home_end( int setting );
//! If true, HOME and END keys will move to the first/last non-space character
//! of the line before moving to the start/end.
//!
//!
