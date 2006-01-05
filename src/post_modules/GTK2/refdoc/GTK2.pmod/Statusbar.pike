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
//!@expr{ lambda() {object sb = GTK2.Statusbar();int id = sb->get_context_id("test");sb->push(id,"A message");sb->push(id,"Another message");return sb;}()@}
//!@xml{<image>../images/gtk2_statusbar.png</image>@}
//!
//!@expr{ lambda() {object sb = GTK2.Statusbar();int id = sb->get_context_id("test");sb->push(id,"A message");sb->push(id,"Another message");sb->pop(id);return sb;}()@}
//!@xml{<image>../images/gtk2_statusbar_2.png</image>@}
//!
//! 
//! Properties:
//! int has-resize-grip
//! 
//! Style properties:
//! int shadow-type
//!
//!
//!  Signals:
//! @b{text_popped@}
//!
//! @b{text_pushed@}
//!

inherit GTK2.Hbox;

static GTK2.Statusbar create( mapping|void props );
//! Create a new statusbar widget
//!
//!

int get_context_id( string context );
//! Create a new context id (or get the id of an old one). The argument
//! is any string. The return value can be used for -&gt;push() -&gt;pop()
//! and -&gt;remove later on.
//!
//!

int get_has_resize_grip( );
//! Returns whether the statusbar has a resize grip.
//!
//!

GTK2.Statusbar pop( int context );
//! Remove the topmost message.
//!
//!

int push( int context, string data );
//! Push a message onto the statusbar. The return value is an id that
//! can be passed to remove later on.
//!
//!

GTK2.Statusbar remove( int context, int id );
//! Remove the specified message (the message id is the second argument).
//!
//!

GTK2.Statusbar set_has_resize_grip( int setting );
//! Sets whether the statusbar has a resize grip.  TRUE by default.
//!
//!
