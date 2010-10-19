//!

inherit GTK.Container;

GTK.Packer add( GTK.Widget widget, int side, int anchor, int options, int border_width, int pad_x, int pad_y, int i_pad_x, int i_pad_y );
//! side is one of @[SIDE_BOTTOM], @[SIDE_LEFT], @[SIDE_RIGHT] and @[SIDE_TOP], anchor is one of @[ANCHOR_CENTER], @[ANCHOR_E], @[ANCHOR_EAST], @[ANCHOR_N], @[ANCHOR_NE], @[ANCHOR_NORTH], @[ANCHOR_NORTH_EAST], @[ANCHOR_NORTH_WEST], @[ANCHOR_NW], @[ANCHOR_S], @[ANCHOR_SE], @[ANCHOR_SOUTH], @[ANCHOR_SOUTH_EAST], @[ANCHOR_SOUTH_WEST], @[ANCHOR_SW], @[ANCHOR_W] and @[ANCHOR_WEST],
//! options is a bitwise or of GTK.PackExpand, GTK.FillX and GTK.FillY
//!
//!

GTK.Packer add_defaults( GTK.Widget widget, int side, int anchor, int options );
//! side is one of @[SIDE_BOTTOM], @[SIDE_LEFT], @[SIDE_RIGHT] and @[SIDE_TOP], anchor is one of @[ANCHOR_CENTER], @[ANCHOR_E], @[ANCHOR_EAST], @[ANCHOR_N], @[ANCHOR_NE], @[ANCHOR_NORTH], @[ANCHOR_NORTH_EAST], @[ANCHOR_NORTH_WEST], @[ANCHOR_NW], @[ANCHOR_S], @[ANCHOR_SE], @[ANCHOR_SOUTH], @[ANCHOR_SOUTH_EAST], @[ANCHOR_SOUTH_WEST], @[ANCHOR_SW], @[ANCHOR_W] and @[ANCHOR_WEST],
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
//! side is one of @[SIDE_BOTTOM], @[SIDE_LEFT], @[SIDE_RIGHT] and @[SIDE_TOP], anchor is one of @[ANCHOR_CENTER], @[ANCHOR_E], @[ANCHOR_EAST], @[ANCHOR_N], @[ANCHOR_NE], @[ANCHOR_NORTH], @[ANCHOR_NORTH_EAST], @[ANCHOR_NORTH_WEST], @[ANCHOR_NW], @[ANCHOR_S], @[ANCHOR_SE], @[ANCHOR_SOUTH], @[ANCHOR_SOUTH_EAST], @[ANCHOR_SOUTH_WEST], @[ANCHOR_SW], @[ANCHOR_W] and @[ANCHOR_WEST],
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
