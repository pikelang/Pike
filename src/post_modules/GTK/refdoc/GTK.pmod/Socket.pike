//! Together with W(Plug), GTK.Socket provides the ability to embed
//! widgets from one process into another process in a fashion that is
//! transparent to the user. One process creates a GTK.Socket widget
//! and, passes the XID of that widget's window to the other process,
//! which then creates a W(Plug) window with that XID. Any widgets
//! contained in the W(Plug) then will appear inside the first
//! applications window.
//! 
//! Note that if you pass the XID of the socket to another process
//! that will create a plug in the socket, you must make sure that the
//! socket widget is not destroyed until that plug is
//! created. Violating this rule will cause unpredictable
//! consequences, the most likely consequence being that the plug will
//! appear as a separate toplevel window.
//! 
//!  A socket can also be used to swallow arbitrary pre-existing
//!  top-level windows using steal(), though the integration when this
//!  is done will not be as close as between a W(Plug) and a
//!  GTK.Socket.
//! 
//!
//!
inherit Container;

static Socket create( )
//!

int get_same_app( )
//! return 1 if the widow contained in this socket comes from this
//! process.
//!
//!

int has_plug( )
//! Returns true if this socket is occupied
//!
//!

int id( )
//! Returns the window id, to be sent to the application providing the plug.
//! You must realize this widget before calling this function.
//!
//!

Socket steal( int window_id )
//! Reparents a pre-existing toplevel window (not nessesarily a GTK
//! window) into a socket.
//!
//!
