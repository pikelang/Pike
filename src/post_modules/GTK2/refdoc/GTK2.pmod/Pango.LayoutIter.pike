//! PangoLayoutIter.
//!
//!

int at_last_line( );
//! Determines whether this iter is on the last line of the layout.
//!
//!

Pango.LayoutIter destroy( );
//! Destructor.
//!
//!

int get_baseline( );
//! Gets the y position of the current line's baseline, in layout coordinates
//! (origin at top left of the entire layout).
//!
//!

mapping get_char_extents( );
//! Gets the extents of the current character, in layout coordinates (origin
//! is the top left of the entire layout).
//!
//!

array get_cluster_extents( );
//! Gets the extents of the current cluster, in layout coordinates.
//!
//!

int get_index( );
//! Gets the current byte index.  Note that iterating forward by char moves in
//! visual order, not logical order, so indexes may not be sequential.  Also,
//! the index may be equal to the length of the text in the layout.
//!
//!

array get_layout_extents( );
//! Obtains the extents of the layout.
//!
//!

GTK2.Pango.LayoutLine get_line( );
//! Gets the current line.
//!
//!

array get_line_extents( );
//! Obtains the extents of the current line.
//!
//!

array get_line_yrange( );
//! Divides the vertical space being iterated over between the lines in the
//! layout, and returns the space belonging to the current line.  A line's
//! range includes the line's logical extents, plus half of the spacing above
//! and below the line, if Pango.Layout->set_spacing() has been called to set 
//! layout spacing.  The y positions are in layout coordinates.
//!
//!

array get_run_extents( );
//! Gets the extents of the current run in layout coordinates.
//!
//!

int next_char( );
//! Moves forward to the next character in visual order.  If it was already at
//! the end of the layout, returns false.
//!
//!

int next_cluster( );
//! Moves forward to the next cluster in visual order.  If it was already at
//! the end of the layout, returns false.
//!
//!

int next_line( );
//! Moves forward to the start of the next line.  If it is already on the last
//! line, returns false.
//!
//!

int next_run( );
//! Moves forward to the next run in visual order.  If it was already at the
//! end of the layout, returns false.
//!
//!
