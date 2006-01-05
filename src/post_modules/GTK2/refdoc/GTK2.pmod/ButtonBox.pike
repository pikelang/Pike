//! More or less equivalent to a normal box, but you can set a few
//! layout schemes that are not available for normal boxes.
//! See the hbox and vbox documentation for examples.
//! Properties:
//! int layout-style
//! 
//! Child properties:
//! int secondary
//! 
//! Style properties:
//! int child-internal-pad-x
//! int child-internal-pad-y
//! int child-min-height
//! int child-min-width
//!
//!

inherit GTK2.Box;

int get_layout( );
//! Returns the currently configured layout.
//! One of  @[BUTTONBOX_DEFAULT_STYLE], @[BUTTONBOX_EDGE], @[BUTTONBOX_END], @[BUTTONBOX_SPREAD] and @[BUTTONBOX_START]
//!
//!

GTK2.ButtonBox set_child_secondary( GTK2.Widget child, int is_secondary );
//! Sets whether child should appear in a secondary group of children.
//!
//!

GTK2.ButtonBox set_layout( int layout );
//! layout is one of @[BUTTONBOX_DEFAULT_STYLE], @[BUTTONBOX_EDGE], @[BUTTONBOX_END], @[BUTTONBOX_SPREAD] and @[BUTTONBOX_START]
//!
//!
