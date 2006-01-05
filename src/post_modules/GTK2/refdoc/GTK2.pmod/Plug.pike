//! Together with W(Socket), GTK2.Plug provides the ability to embed
//! widgets from one process into another process in a fashion that is
//! transparent to the user. One process creates a W(Socket) widget
//! and, passes the XID of that widgets window to the other process,
//! which then creates a GTK2.Plug window with that XID. Any widgets
//! contained in the GTK2.Plug then will appear inside the first
//! applications window.
//!
//!
//!  Signals:
//! @b{embedded@}
//!

inherit GTK2.Window;

static GTK2.Plug create( int|mapping socket_id_or_props );
//! Create a new plug, the socket_id is the window into which this plug
//! will be plugged.
//!
//!

int get_id( );
//! Gets the window id of this widget.
//!
//!
