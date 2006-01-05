//! Anchors for embedding widgets in a TextBuffer.
//!
//!

inherit G.Object;

static GTK2.TextChildAnchor create( );
//! Creates a new W(TextChildAnchor).  Usually you would then insert it into
//! W(TextBuffer) with W(TextBuffer)->insert_child_anchor().  To perform the
//! creation and insertion in one step, use the convenience function
//! W(TextBuffer)->create_child_anchor().
//!
//!

int get_deleted( );
//! Determines whether a child anchor has been deleted from the buffer.
//!
//!

array get_widgets( );
//! Gets a list of all widgets anchored at this child anchor.
//!
//!
