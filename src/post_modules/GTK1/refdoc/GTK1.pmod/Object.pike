//! The basic GTK class.
//! All other GTK classes inherit this class.
//! The only user-callable functions are the signal related ones.
//!
//!
//!  Signals:
//! @b{destroy@}
//! Called when the object is destroyed
//!
//!

GTK.Object destroy( );
//!

GTK.Object signal_block( mixed signal_id );
//! Temporarily block a signal handler. No signals will be received
//! while the hander is blocked.
//! See signal connect for more info.
//!
//!

mixed signal_connect( string signal, function callback, mixed|void callback_arg );
//! Connect a signal to a pike function. The function will be called with
//! the last argument to this function as it's first argument (defaults
//! to 0), the second argument is always the widget, any other
//! arguments are the ones supplied by GTK.
//! 
//! The return value of this function can be used to remove a signal
//! with signal_disconnect, and block and unblock the signal will
//! signal_block and signal_unblock.
//! 
//!
//!

mixed signal_connect_new( string signal, function callback, mixed|void callback_arg );
//! Connect a signal to a pike function.
//! 
//! 
//! 
//! This function differs from the signal_connect function in how it
//! calls the callback function.
//!
//! 
//!
//! 
//! The old interface:
//! @pre{
//!   void signal_handler( mixed my_arg, GTK.Object object,
//!                        mixed ... signal_arguments )
//! @}
//! The new interface:
//! @pre{
//!   void signal_handler( mixed ... signal_arguments,
//!                        mixed my_arg, GTK.Object object )
//! @}
//! 
//! The return value of this function can be used to remove a signal
//! with signal_disconnect, and block and unblock the signal will
//! signal_block and signal_unblock.
//! 
//!
//!

GTK.Object signal_disconnect( mixed signal_id );
//! Remove a signal formerly added by signal_connect. The argument is
//! the return value of signal_connect(). See signal connect for more info.
//!
//!

GTK.Object signal_emit( string signal_name );
//! Halt the emit of the current named signal.
//! Useful in signal handlers when you want to override the behaviour
//! of some default signal handler (key press events, as an example)
//! See signal_connect.
//!
//!

GTK.Object signal_unblock( mixed signal_id );
//! Unblock a formerly blocked signal handler. See signal_block and
//! signal_connect for more info.
//!
//!
