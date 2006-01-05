//! Properties:
//! int draw
//!
//!

inherit GTK2.ToolItem;

static GTK2.SeparatorToolItem create( mapping|void props );
//! Create a new GTK2.SeparatorToolItem.
//!
//!

int get_draw( );
//! Returns whether SeparatorToolItem is drawn as a line,
//! or just a blank
//!
//!

GTK2.SeparatorToolItem set_draw( int draw );
//! When a SeparatorToolItem is drawn as a line,
//! or just a blank.
//!
//!
