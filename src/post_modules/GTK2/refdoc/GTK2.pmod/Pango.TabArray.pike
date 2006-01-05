//!

inherit G.Object;

GTK2.Pango.TabArray copy( );
//! Returns a copy.
//!
//!

static Pango.TabArray create( int initial_size, int position_in_pixels );
//! Creates an array of initial_size tab stops.  Tab stops are specified in
//! pixel units if positions_in_pixels is true, otherwise in Pango units. All
//! stops are initially at position 0.
//!
//!

Pango.TabArray destroy( );
//! Destructor.
//!
//!

int get_positions_in_pixels( );
//! Returns true if the tab positions are in pixels, false if they are in
//! Pango units.
//!
//!

int get_size( );
//! Gets the number of tab stops.
//!
//!

mapping get_tab( int tab_index );
//! Gets the alignment and position of a tab stop.
//!
//!

array get_tabs( );
//! Gets alignments and locations of all tab stops.
//!
//!

Pango.TabArray resize( int new_size );
//! Resizes the array.  You must subsequently initialize any tabs that were
//! added as a result of growing the array.
//!
//!

Pango.TabArray set_tab( int tab_index, int alignment, int location );
//! Sets the alignment and location of a tab stop.  Alignment must always be
//! @[PANGO_TAB_LEFT].
//!
//!
