//! Statusbars are simple widgets used to display a text message. They
//! keep a stack of the messages pushed onto them, so that popping the
//! current message will re-display the previous text message.
//!  
//! In order to allow different parts of an application to use the same
//! statusbar to display messages, the statusbar widget issues Context
//! Identifiers which are used to identify different 'users'. The
//! message on top of the stack is the one displayed, no matter what
//! context it is in. Messages are stacked in last-in-first-out order,
//! not context identifier order.
//!  
//!@code{ lambda() {object sb = GTK.Statusbar();int id = sb->get_context_id("test");sb->push(id,"A message");sb->push(id,"Another message");return sb;}()@}
//!@xml{<image>../images/gtk_statusbar.png</image>@}
//!
//!@code{ lambda() {object sb = GTK.Statusbar();int id = sb->get_context_id("test");sb->push(id,"A message");sb->push(id,"Another message");sb->pop(id);return sb;}()@}
//!@xml{<image>../images/gtk_statusbar_2.png</image>@}
//!
//! 
//!
//!
//!  Signals:
//! @b{text_poped@}
//!
//! @b{text_pushed@}
//!

inherit GTK.Hbox;

static GTK.Statusbar create( );
//! Create a new statusbar widget
//!
//!

int get_context_id( string context );
//! Create a new context id (or get the id of an old one). The argument
//! is any string. The return value can be used for -&gt;push() -&gt;pop()
//! and -&gt;remove later on.
//!
//!

GTK.Statusbar pop( int context );
//! Remove the topmost message.
//!
//!

int push( int context, string data );
//! Push a message onto the statusbar. The return value is an id that
//! can be passed to remove later on.
//!
//!

GTK.Statusbar remove( int context, int id );
//! Remove the specified message (the message id is the second argument).
//!
//!
