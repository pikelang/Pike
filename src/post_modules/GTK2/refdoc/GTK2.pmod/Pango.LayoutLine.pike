//! PangoLayoutLine.
//!
//!

array get_extents( );
//! Computes the logical and ink extents of a layout line.
//!
//!

array get_pixel_extents( );
//! Computes the logical and ink extents of a layout line, in device units.
//!
//!

int index_to_x( int index, int trailing );
//! Converts an index within a line to an x position.
//!
//!

mapping x_to_index( int x_pos );
//! Converts from x offset to the byte index of the corresponding character
//! within the text of the layout.  If x_pos is outside the line, index and
//! trailing will point to the very first or very last position in the line.
//! This determination is based on the resolved direction of the paragraph;
//! for example, if the resolved direction is right-to-left, then an X position
//! to the right of the line (after it) results in 0 being stored in index and
//! trailing.  An X position to the left of the line results in index pointing
//! to the (logical) last grapheme in the line and trailing set to the number
//! of characters in that grapheme.  The reverse is true for a left-to-right
//! line.
//!
//!
