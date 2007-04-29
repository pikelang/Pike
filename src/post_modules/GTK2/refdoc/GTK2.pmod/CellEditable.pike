//!

GTK2.CellEditable editing_done( );
//! Emits the "editing-done" signal.  This signal is a sign for the cell
//! renderer to update its value from the cell.
//!
//!

GTK2.CellEditable remove_widget( );
//! Emits the "remove-widget" signal.  This signal is meant to indicate that
//! the cell is finished editing, and the widget may now be destroyed.
//!
//!

GTK2.CellEditable start_editing( GTK2.GdkEvent event );
//! Begins editing.  event is the GDK2.Event that began the editing process.
//! It may be empty, in the instance that editing was initiated through
//! programmatic means.
//!
//!
