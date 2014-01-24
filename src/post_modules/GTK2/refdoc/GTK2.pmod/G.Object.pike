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
//! Destroy this object. This is the normal way to (eg) close a window.
//!
//!

object get_data( string name );
//! Gets a named field from the object.
//!
//!

string get_docs( );
//! Get documentation on object
//!
//!

mixed get_property( string property );
//! Get a property.
//!
//!

int new_signal( string name, array types, string return_type );
//! Create a new signal.
//!
//!

G.Object notify( string property );
//! Emits a "notify" signal for the named property on the object.
//!
//!

object set_data( string name, mixed arg );
//! Each object carries around a table of associations from strings to
//! pointers. This function lets you set an association.
//!
//! If the object already had an association with that name, the old
//! association will be destroyed.
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

int signal_connect( string signal, function callback, mixed|void callback_arg, string|void detail, int|void connect_before );
//! Connect a signal to a pike function.  The function will be called with
//! the last argument to this function as its last argument (defaults to 0);
//! the first argument is always the widget, and any other arguments are the
//! ones supplied by GTK. If connect_before is nonzero, the callback will be
//! called prior to the normal handling of the signal (and can return true
//! to suppress that handling), otherwise it will be called after.
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

G.Object signal_emit( string signal_name, string|void detail );
//! Send the current named signal.
//!
//!

G.Object signal_unblock( int signal_id );
//! Unblock a formerly blocked signal handler.  See signal_block() and
//! signal_connect() for more info.
//!
//!
