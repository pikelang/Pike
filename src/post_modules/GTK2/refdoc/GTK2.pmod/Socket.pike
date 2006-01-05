//! Together with W(Plug), GTK2.Socket provides the ability to embed
//! widgets from one process into another process in a fashion that is
//! transparent to the user. One process creates a GTK2.Socket widget
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
//!  GTK2.Socket.
//! 
//!
//!
//!  Signals:
//! @b{plug_added@}
//!
//! @b{plug_removed@}
//!

inherit GTK2.Container;

GTK2.Socket add_id( int wid );
//! Adds an XEMBED client, such as a W(Plug), to the W(Socket).
//!
//!

static GTK2.Socket create( mapping|void props );
//! Create a new GTK2.Socket.
//!
//!

int get_id( );
//! Gets the window id of a W(Socket) widget, which can then be used
//! to create a client embedded inside the socket, for instance with
//! GTK2.Plug->create().
//!
//!

int id( );
//! Returns the window id, to be sent to the application providing the plug.
//! You must realize this widget before calling this function.
//!
//!
