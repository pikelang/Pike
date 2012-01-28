//! A box is a container that can contain more than one child.
//! The basic 'Box' class cannot be instantiated, it is a virtual class
//! that only defines some common 'Box' functions shared with all other
//! Box widgets.
//!
//!

inherit GTK1.Container;

GTK1.Box pack_end( GTK1.Widget widget, int expandp, int fillp, int padding );
//! Pack from the right (or bottom) of the box.
//! Arguments are widget, expand, fill, paddingb
//! widget, expand, fill, padding
//!
//!

GTK1.Box pack_end_defaults( GTK1.Widget widget );
//! The argument is the widget to add.
//!
//!

GTK1.Box pack_start( GTK1.Widget widget, int expandp, int fillp, int padding );
//! Pack from the left (or top) of the box.
//! Argument are widget, expand, fill, padding
//! pack(widget,1,1,0) is equivalent to 'add' or 'pack_start_defaults'
//!
//!

GTK1.Box pack_start_defaults( GTK1.Widget widget );
//! The argument is the widget to add. This function is equivalent to 'add'
//!
//!

mapping query_child_packing( GTK1.Widget child );
//! Return a mapping:
//! ([ "expand":expandp, "fill":fillp, "padding":paddingp, "type":type ])
//!
//!

GTK1.Box reorder_child( GTK1.Widget child, int new_position );
//! Move widget to pos, pos is an integer,
//! between 0 and sizeof(box->children())-1
//!
//!

GTK1.Box set_child_packing( GTK1.Widget child_widget, int expandp, int fillp, int padding, int pack_type );
//! widget, expand, fill, padding, pack_type.
//! If exand is true, the widget will be expanded when the box is resized.
//! If 'fill' is true, the widget will be resized to fill up all available
//! space. Padding is the amount of padding to use, and pack_type is
//! one of @[PACK_END], @[PACK_EXPAND] and @[PACK_START].
//! 
//! You can emulate pack_start and pack_end with add and set_child_packing.
//!
//!

GTK1.Box set_homogeneous( int homogeneousp );
//! If true, all widgets in the box will get exactly the same amount of space
//!
//!

GTK1.Box set_spacing( int spacing );
//! This is the amount of spacing (in pixels) inserted beween all widgets
//!
//!
