//! Marks for the text.
//!
//!

inherit G.Object;

GTK2.TextMark destroy( );
//! Destructor.
//!
//!

GTK2.TextBuffer get_buffer( );
//! Gets the buffer this mark is located inside, or empty if the mark
//! is deleted.
//!
//!

int get_deleted( );
//! Returns true if the mark has been removed from its buffer with
//! delete_mark().  Marks can't be used once deleted.
//!
//!

int get_left_gravity( );
//! Determines whether the mark has left gravity.
//!
//!

string get_name( );
//! Returns the mark name;  returns empty for anonymous marks.
//!
//!

int get_visible( );
//! Returns true if the mark is visible.
//!
//!

GTK2.TextMark set_visible( int setting );
//! Sets the visibility of the mark; the insertion point is normally
//! visible, i.e. you can see it as a vertical bar.  Also the text
//! widget uses a visible mark to indicate where a drop will occur when
//! dragging-and-dropping text.  Most other marks are not visible.
//! Marks are not visible by default.
//!
//!
