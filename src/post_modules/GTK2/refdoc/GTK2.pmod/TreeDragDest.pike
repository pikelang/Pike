//!

int drag_data_received( GTK2.TreePath path, GTK2.SelectionData sel );
//! Asks to insert a row before the path dest, deriving the contents of the
//! row from the sel.  If this dest is outside the tree so that inserting
//! before it is impossible, false will be returned.  Also, false may be
//! returned if the new row is not created for some model-specific reason.
//!
//!

int row_drop_possible( GTK2.TreePath path, GTK2.SelectionData sel );
//! Determines whether a drop is possible before past, at the same depth as
//! path.  i.e., can we drop the data in sel at that location.  path does not
//! have to exist; the return value will almost certainly be false if the
//! parent of path doesn't exist, though.
//!
//!
