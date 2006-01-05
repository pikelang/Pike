//! The drag context contains all information about the drag'n'drop
//! connected to the signal to which it is an argument.
//! 
//! NOIMG
//! 
//!
//!

GDK2.DragContext drag_abort( int time );
//! Abort the drag
//!
//!

GDK2.DragContext drag_drop( int time );
//! Drag drop.
//!
//!

GDK2.DragContext drag_finish( int success, int del );
//! If success is true, the drag succeded.
//! If del is true, the source should be deleted.
//! time is the current time.
//!
//!

GDK2.DragContext drag_set_icon_default( );
//! Use the default drag icon associated with the source widget.
//!
//!

GDK2.DragContext drag_set_icon_pixmap( GTK2.GdkPixmap p, GTK2.GdkBitmap b, int hot_x, int hot_y );
//! Set the drag pixmap, and optionally mask.
//! The hot_x and hot_y coordinates will be the location of the mouse pointer,
//! relative to the upper left corner of the pixmap.
//!
//!

GDK2.DragContext drag_set_icon_widget( GTK2.Widget widget, int hot_x, int hot_y );
//! Set the drag widget. This is a widget that will be shown, and then
//! dragged around by the user during this drag.
//!
//!

GDK2.DragContext drag_status( int action );
//! Setting action to -1 means use the suggested action
//!
//!

GDK2.DragContext drop_reply( int ok );
//! Drop reply.
//!
//!

int get_action( );
//! One of @[GDK_ACTION_ASK], @[GDK_ACTION_COPY], @[GDK_ACTION_DEFAULT], @[GDK_ACTION_LINK], @[GDK_ACTION_MOVE] and @[GDK_ACTION_PRIVATE];
//!
//!

int get_actions( );
//! A bitwise or of one or more of @[GDK_ACTION_ASK], @[GDK_ACTION_COPY], @[GDK_ACTION_DEFAULT], @[GDK_ACTION_LINK], @[GDK_ACTION_MOVE] and @[GDK_ACTION_PRIVATE];
//!
//!

int get_is_source( );
//! Is this application the source?
//!
//!

int get_protocol( );
//! One of @[GDK_DRAG_PROTO_LOCAL], @[GDK_DRAG_PROTO_MOTIF], @[GDK_DRAG_PROTO_NONE], @[GDK_DRAG_PROTO_OLE2], @[GDK_DRAG_PROTO_ROOTWIN], @[GDK_DRAG_PROTO_WIN32_DROPFILES] and @[GDK_DRAG_PROTO_XDND]
//!
//!

GTK2.Widget get_source_widget( );
//! Return the drag source widget.
//!
//!

int get_start_time( );
//! The start time of this drag, as a unix time_t (seconds since 0:00 1/1 1970)
//!
//!

int get_suggested_action( );
//! One of @[GDK_ACTION_ASK], @[GDK_ACTION_COPY], @[GDK_ACTION_DEFAULT], @[GDK_ACTION_LINK], @[GDK_ACTION_MOVE] and @[GDK_ACTION_PRIVATE];
//!
//!
