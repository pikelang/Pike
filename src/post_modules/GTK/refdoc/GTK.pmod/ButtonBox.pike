//! More or less equivalent to a normal box, but you can set a few
//! layout schemes that are not available for normal boxes.
//! See the hbox and vbox documentation for examples.
//!
//!

inherit GTK.Box;

mapping get_child_ipadding( );
//! Return the default inter-child padding ([ "x":xpadding, "y":ypadding ])
//!
//!

mapping get_child_size( );
//! Return the child size as ([ "x":xsize, "y":ysize ])
//!
//!

int get_layout( );
//! Returns the currently configured layout.
//! One of  @[BUTTONBOX_DEFAULT_STYLE], @[BUTTONBOX_EDGE], @[BUTTONBOX_END], @[BUTTONBOX_SPREAD] and @[BUTTONBOX_START]
//!
//!

int get_spacing( );
//! Return the spacing that is added between the buttons
//!
//!

GTK.ButtonBox set_child_ipadding( int child_number, int child_padding );
//! Set the padding for a specific child.
//!
//!

GTK.ButtonBox set_child_size( int child_number, int child_size );
//! Set the size of a specified child
//!
//!

GTK.ButtonBox set_layout( int layout );
//! layout is one of @[BUTTONBOX_DEFAULT_STYLE], @[BUTTONBOX_EDGE], @[BUTTONBOX_END], @[BUTTONBOX_SPREAD] and @[BUTTONBOX_START]
//!
//!

GTK.ButtonBox set_spacing( int spacing );
//! in pixels
//!
//!
