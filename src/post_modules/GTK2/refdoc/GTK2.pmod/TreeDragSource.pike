//!

int drag_data_delete( GTK2.TreePath path );
//! Asks to delete the row at path, because it was moved somewhere else via
//! drag-and-drop.  Returns false if the deletion fails because path no longer
//! exists, or for some model-specific reason.
//!
//!

GTK2.SelectionData drag_data_get( GTK2.TreePath path );
//! Asks to return a representation of the row at path.
//!
//!

int row_draggable( GTK2.TreePath path );
//! Asks the source whether a particular row can be used as the source of a
//! DND operation.  If the source doesn't implement this interface, the row
//! is assumed draggable.
//!
//!
