//!

inherit Container;

Packer add( GTK.Widget widget, int side, int anchor, int options, int border_width, int pad_x, int pad_y, int i_pad_x, int i_pad_y );
//! side is one of @[SIDE_TOP], @[SIDE_RIGHT], @[SIDE_LEFT] and @[SIDE_BOTTOM], anchor is one of @[ANCHOR_E], @[ANCHOR_SW], @[ANCHOR_WEST], @[ANCHOR_W], @[ANCHOR_NORTH_WEST], @[ANCHOR_S], @[ANCHOR_SOUTH], @[ANCHOR_NE], @[ANCHOR_CENTER], @[ANCHOR_NORTH_EAST], @[ANCHOR_NORTH], @[ANCHOR_NW], @[ANCHOR_SOUTH_WEST], @[ANCHOR_N], @[ANCHOR_SE], @[ANCHOR_SOUTH_EAST] and @[ANCHOR_EAST],
//! options is a bitwise or of GTK.PackExpand, GTK.FillX and GTK.FillY
//!
//!

Packer add_defaults( GTK.Widget widget, int side, int anchor, int options );
//! side is one of @[SIDE_TOP], @[SIDE_RIGHT], @[SIDE_LEFT] and @[SIDE_BOTTOM], anchor is one of @[ANCHOR_E], @[ANCHOR_SW], @[ANCHOR_WEST], @[ANCHOR_W], @[ANCHOR_NORTH_WEST], @[ANCHOR_S], @[ANCHOR_SOUTH], @[ANCHOR_NE], @[ANCHOR_CENTER], @[ANCHOR_NORTH_EAST], @[ANCHOR_NORTH], @[ANCHOR_NW], @[ANCHOR_SOUTH_WEST], @[ANCHOR_N], @[ANCHOR_SE], @[ANCHOR_SOUTH_EAST] and @[ANCHOR_EAST],
//! options is a bitwise or of GTK.PackExpand, GTK.FillX and GTK.FillY
//!
//!

static Packer create( );
//!

int get_default_border_width( );
//!

int get_default_i_pad_x( );
//!

int get_default_i_pad_y( );
//!

int get_default_pad_x( );
//!

int get_default_pad_y( );
//!

int get_spacing( );
//!

Packer reorder_child( GTK.Widget child, int pos );
//!

Packer set_child_packing( GTK.Widget child, int side, int anchor, int options, int border_width, int pad_x, int pad_y, int i_pad_x, int i_pad_y );
//! side is one of @[SIDE_TOP], @[SIDE_RIGHT], @[SIDE_LEFT] and @[SIDE_BOTTOM], anchor is one of @[ANCHOR_E], @[ANCHOR_SW], @[ANCHOR_WEST], @[ANCHOR_W], @[ANCHOR_NORTH_WEST], @[ANCHOR_S], @[ANCHOR_SOUTH], @[ANCHOR_NE], @[ANCHOR_CENTER], @[ANCHOR_NORTH_EAST], @[ANCHOR_NORTH], @[ANCHOR_NW], @[ANCHOR_SOUTH_WEST], @[ANCHOR_N], @[ANCHOR_SE], @[ANCHOR_SOUTH_EAST] and @[ANCHOR_EAST],
//! options is a bitwise or of GTK.PackExpand, GTK.FillX and GTK.FillY
//!
//!

Packer set_default_border_width( int border );
//!

Packer set_default_ipad( int xpad, int ypad );
//!

Packer set_default_pad( int xpad, int ypad );
//!

Packer set_spacing( int new_spacing );
//!
