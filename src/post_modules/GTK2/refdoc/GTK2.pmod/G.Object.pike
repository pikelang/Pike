//! The base object.  All GDK and GTK classes inherit from this.
//! The Pango classes also inherit from this class.
//!
//!
//!  Signals:
//! @b{notify@}
//!

G.Object accel_groups_activate( int accel_key, int accel_mods );
//! Finds the first accelerator in an GTK.AccelGroup attached to this object
//! that matches accel_key and accel_mods, and activates that accelerator.
//!
//!

G.Object destroy( );
//! Destructor.
//!
//!

mixed get_property( string property );
//! Get a property.
//!
//!

G.Object notify( string property );
//! Emits a "notify" signal for the named property on the object.
//!
//!

G.Object set_property( string property, mixed value );
//! Set a property on a G.Object (and any derived types) to value.
//!
//!

G.Object signal_block( int signal_id );
//! Temporarily block a signal handler.  No signals will be received while the
//! handler is blocked.
//! See signal_connect() for more info.
//!
//!

int signal_connect( string signal, function callback, mixed|void callback_arg );
//! Connect a signal to a pike function.  The function will be called with
//! the last argument to this function as it's first argument (defaults to 0),
//! the second argument is always the widget, and any other arguments are the
//! ones supplied by GTK.
//! 
//! The return value of this function can be used to remove a signal with
//! signal_disconnect(), and block and unblock the signal with signal_block()
//! and signal_unblock().
//!
//!

G.Object signal_disconnect( int signal_id );
//! Remove a signal formerly added by signal_connect.  The argument is the
//! return value of signal_connect().  See signal_connect() for more info.
//!
//!

G.Object signal_emit( string signal_name );
//! Send the current named signal.
//!
//!

G.Object signal_unblock( int signal_id );
//! Unblock a formerly blocked signal handler.  See signal_block() and
//! signal_connect() for more info.
//!
//!
