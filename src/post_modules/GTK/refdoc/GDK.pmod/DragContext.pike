//! The drag context contains all information about the drag'n'drop
//! connected to the signal to which it is an argument.
//! 
//! NOIMG
//! 
//!
//!

GDK.DragContext drag_abort( int time );
//! Abort the drag
//!
//!

GDK.DragContext drag_drop( int time );
//!

GDK.DragContext drag_finish( int success, int del, int time );
//! If success is true, the drag succeded.
//! If del is true, the source should be deleted.
//! time is the current time.
//!
//!

GDK.DragContext drag_set_icon_default( );
//! Use the default drag icon associated with the source widget.
//!
//!

GDK.DragContext drag_set_icon_pixmap( GDK.Pixmap p, GDK.Bitmap b, int hot_x, int hot_y );
//! Set the drag pixmap, and optionally mask.
//! The hot_x and hot_y coordinates will be the location of the mouse pointer,
//! relative to the upper left corner of the pixmap.
//!
//!

GDK.DragContext drag_set_icon_widget( GTK.Widget widget, int hot_x, int hot_y );
//! Set the drag widget. This is a widget that will be shown, and then
//! dragged around by the user during this drag.
//!
//!

GDK.DragContext drag_status( int action, int time );
//! Setting action to -1 means use the suggested action
//!
//!

GDK.DragContext drop_reply( int ok, int time );
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
//! One of @[GDK_DRAG_PROTO_MOTIF], @[GDK_DRAG_PROTO_ROOTWIN] and @[GDK_DRAG_PROTO_XDND]
//!
//!

GTK.Widget get_source_widget( );
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
