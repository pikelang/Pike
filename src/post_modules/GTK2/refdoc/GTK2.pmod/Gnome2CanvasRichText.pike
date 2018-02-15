//! Properties:
//! int anchor
//! int cursor-blink
//! int cursor-visible
//! int direction
//! int editable
//! int grow-height
//! float height
//! int indent
//! int justification
//! int left-margin
//! int pixels-above-lines
//! int pixels-below-lines
//! int pixels-inside-wrap
//! int right-margin
//! string text
//! int visible
//! float width
//! int wrap-mode
//! float x
//! float y
//!
//!
//!  Signals:
//! @b{tag_changed@}
//!

inherit Gnome2.CanvasItem;

GTK2.TextBuffer get_buffer( );
//! Get the text buffer.
//!
//!

Gnome2.CanvasRichText set_buffer( GTK2.TextBuffer buffer );
//! Set the text buffer.
//!
//!
