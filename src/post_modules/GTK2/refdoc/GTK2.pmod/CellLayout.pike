//! An interface for packing cells.
//!
//!

GTK2.CellLayout add_attribute( GTK2.CellRenderer cell, string attribute, int column );
//! Adds an attribute mapping.
//!
//!

GTK2.CellLayout clear( );
//! Unsets all the mappings on all renderers and removes all renderers.
//!
//!

GTK2.CellLayout clear_attributes( GTK2.CellRenderer cell );
//! Clears all existing attributes.
//!
//!

GTK2.CellLayout pack_end( GTK2.CellRenderer cell, int expand );
//! See pack_start().
//!
//!

GTK2.CellLayout pack_start( GTK2.CellRenderer cell, int expand );
//! Packs the cell into the beginning.  If expand is false, then the cell is
//! allocated no more space than it needs.  Any unused space is divied evenly
//! between the cells for which expand is true.
//!
//!

GTK2.CellLayout reorder( GTK2.CellRenderer cell, int position );
//! Re-inserts cell at position.
//!
//!

GTK2.CellLayout set_cell_data_func( GTK2.CellRenderer cell, function f, mixed user_data );
//! Sets the callback to use for this cell layout.  This function is used
//! instead of the standard attributes mapping for setting the column
//! value, and should set the value of the cell layout's cell renderer(s)
//! as appropriate.  f may be 0 to remove an older one.
//!
//!
