//! The basic GTK class.
//! All other GTK classes inherit this class.
//! The only user-callable functions are the signal related ones.
//! Properties:
//! gpointer user-data
//!
//!
//!  Signals:
//! @b{destroy@}
//!

inherit G.Object;

GTK2.Object destroy( );
//! Destructor.
//!
//!
