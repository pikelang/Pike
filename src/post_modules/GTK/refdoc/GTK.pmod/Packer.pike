//!

inherit GTK.Container;

GTK.Packer add( GTK.Widget widget, int side, int anchor, int options, int border_width, int pad_x, int pad_y, int i_pad_x, int i_pad_y );
//! side is one of @[SIDE_BOTTOM], @[SIDE_TOP], @[SIDE_RIGHT] and @[SIDE_LEFT], anchor is one of @[ANCHOR_SOUTH_WEST], @[ANCHOR_EAST], @[ANCHOR_SW], @[ANCHOR_WEST], @[ANCHOR_SOUTH_EAST], @[ANCHOR_CENTER], @[ANCHOR_S], @[ANCHOR_SOUTH], @[ANCHOR_NE], @[ANCHOR_NORTH_WEST], @[ANCHOR_NORTH], @[ANCHOR_E], @[ANCHOR_NW], @[ANCHOR_NORTH_EAST], @[ANCHOR_W], @[ANCHOR_N] and @[ANCHOR_SE],
//! options is a bitwise or of GTK.PackExpand, GTK.FillX and GTK.FillY
//!
//!

GTK.Packer add_defaults( GTK.Widget widget, int side, int anchor, int options );
//! side is one of @[SIDE_BOTTOM], @[SIDE_TOP], @[SIDE_RIGHT] and @[SIDE_LEFT], anchor is one of @[ANCHOR_SOUTH_WEST], @[ANCHOR_EAST], @[ANCHOR_SW], @[ANCHOR_WEST], @[ANCHOR_SOUTH_EAST], @[ANCHOR_CENTER], @[ANCHOR_S], @[ANCHOR_SOUTH], @[ANCHOR_NE], @[ANCHOR_NORTH_WEST], @[ANCHOR_NORTH], @[ANCHOR_E], @[ANCHOR_NW], @[ANCHOR_NORTH_EAST], @[ANCHOR_W], @[ANCHOR_N] and @[ANCHOR_SE],
//! options is a bitwise or of GTK.PackExpand, GTK.FillX and GTK.FillY
//!
//!

static GTK.Packer create( );
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

GTK.Packer reorder_child( GTK.Widget child, int pos );
//!

GTK.Packer set_child_packing( GTK.Widget child, int side, int anchor, int options, int border_width, int pad_x, int pad_y, int i_pad_x, int i_pad_y );
//! side is one of @[SIDE_BOTTOM], @[SIDE_TOP], @[SIDE_RIGHT] and @[SIDE_LEFT], anchor is one of @[ANCHOR_SOUTH_WEST], @[ANCHOR_EAST], @[ANCHOR_SW], @[ANCHOR_WEST], @[ANCHOR_SOUTH_EAST], @[ANCHOR_CENTER], @[ANCHOR_S], @[ANCHOR_SOUTH], @[ANCHOR_NE], @[ANCHOR_NORTH_WEST], @[ANCHOR_NORTH], @[ANCHOR_E], @[ANCHOR_NW], @[ANCHOR_NORTH_EAST], @[ANCHOR_W], @[ANCHOR_N] and @[ANCHOR_SE],
//! options is a bitwise or of GTK.PackExpand, GTK.FillX and GTK.FillY
//!
//!

GTK.Packer set_default_border_width( int border );
//!

GTK.Packer set_default_ipad( int xpad, int ypad );
//!

GTK.Packer set_default_pad( int xpad, int ypad );
//!

GTK.Packer set_spacing( int new_spacing );
//!
