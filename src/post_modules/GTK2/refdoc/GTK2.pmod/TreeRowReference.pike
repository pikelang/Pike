//! A TreeRowReference.
//!
//!

GTK2.TreeRowReference copy( );
//! Copies a GTK2.TreeRowReference.
//!
//!

static GTK2.TreeRowReference create( GTK2.TreeModel model, GTK2.TreePath path );
//! Creates a row reference based on path.  This reference
//! will keep pointing to the node pointed to by path, so
//! long as it exists.
//!
//!

GTK2.TreeRowReference destroy( );
//! Destructor.
//!
//!

GTK2.TreeModel get_model( );
//! Returns the model which this references is monitoring.
//!
//!

GTK2.TreePath get_path( );
//! Returns a path that the row reference currently points to.
//!
//!

int valid( );
//! Return TRUE if the reference referes to a current valid path.
//!
//!
