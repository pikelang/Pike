//! The data associated with a selection.
//!
//!

string data( );
//! Returns the selection in the data.
//! The return value is always a string, but the width can vary (8, 16
//! or 32 bits per character).
//!
//!

int format( );
//! Returns the selction format.
//! The format is the number of bits per character.
//!
//!

int length( );
//! Return the size of the selection data, in bytes.
//! 
//! The size of the data in characters (as returned by data()) is not
//! necessarily the same.
//! 
//!
//!

object selection( );
//! The selection id, as a GDK(Atom).
//!
//!

SelectionData set( string data );
//!  Store new data into a GtkSelectionData object. Should _only_ by
//!  called from a selection handler callback.
//!
//!

object target( );
//! The target, as a GDK(Atom).
//!
//!

object type( );
//! The selection type, as a GDK(Atom).
//!
//!
