//! More or less equivalent to a normal box, but you can set a few
//! layout schemes that are not available for normal boxes.
//! See the hbox and vbox documentation for examples.
//!
//!
inherit Box;

mapping get_child_ipadding( )
//! Return the default inter-child padding ([ "x":xpadding, "y":ypadding ])
//!
//!

mapping get_child_size( )
//! Return the child size as ([ "x":xsize, "y":ysize ])
//!
//!

int get_layout( )
//! Returns the currently configured layout.
//! One of  @[BUTTONBOX_EDGE], @[BUTTONBOX_START], @[BUTTONBOX_END], @[BUTTONBOX_DEFAULT_STYLE] and @[BUTTONBOX_SPREAD]
//!
//!

int get_spacing( )
//! Return the spacing that is added between the buttons
//!
//!

ButtonBox set_child_ipadding( int child_number, int child_padding )
//! Set the padding for a specific child.
//!
//!

ButtonBox set_child_size( int child_number, int child_size )
//! Set the size of a specified child
//!
//!

ButtonBox set_layout( int layout )
//! layout is one of @[BUTTONBOX_EDGE], @[BUTTONBOX_START], @[BUTTONBOX_END], @[BUTTONBOX_DEFAULT_STYLE] and @[BUTTONBOX_SPREAD]
//!
//!

ButtonBox set_spacing( int spacing )
//! in pixels
//!
//!
