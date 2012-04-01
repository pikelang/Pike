//!

inherit GTK1.Container;

GTK1.Packer add( GTK1.Widget widget, int side, int anchor, int options, int border_width, int pad_x, int pad_y, int i_pad_x, int i_pad_y );
//! side is one of @[SIDE_BOTTOM], @[SIDE_LEFT], @[SIDE_RIGHT] and @[SIDE_TOP], anchor is one of @[ANCHOR_CENTER], @[ANCHOR_E], @[ANCHOR_EAST], @[ANCHOR_N], @[ANCHOR_NE], @[ANCHOR_NORTH], @[ANCHOR_NORTH_EAST], @[ANCHOR_NORTH_WEST], @[ANCHOR_NW], @[ANCHOR_S], @[ANCHOR_SE], @[ANCHOR_SOUTH], @[ANCHOR_SOUTH_EAST], @[ANCHOR_SOUTH_WEST], @[ANCHOR_SW], @[ANCHOR_W] and @[ANCHOR_WEST],
//! options is a bitwise or of GTK1.PackExpand, GTK1.FillX and GTK1.FillY
//!
//!

GTK1.Packer add_defaults( GTK1.Widget widget, int side, int anchor, int options );
//! side is one of @[SIDE_BOTTOM], @[SIDE_LEFT], @[SIDE_RIGHT] and @[SIDE_TOP], anchor is one of @[ANCHOR_CENTER], @[ANCHOR_E], @[ANCHOR_EAST], @[ANCHOR_N], @[ANCHOR_NE], @[ANCHOR_NORTH], @[ANCHOR_NORTH_EAST], @[ANCHOR_NORTH_WEST], @[ANCHOR_NW], @[ANCHOR_S], @[ANCHOR_SE], @[ANCHOR_SOUTH], @[ANCHOR_SOUTH_EAST], @[ANCHOR_SOUTH_WEST], @[ANCHOR_SW], @[ANCHOR_W] and @[ANCHOR_WEST],
//! options is a bitwise or of GTK1.PackExpand, GTK1.FillX and GTK1.FillY
//!
//!

protected GTK1.Packer create( );
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

GTK1.Packer reorder_child( GTK1.Widget child, int pos );
//!

GTK1.Packer set_child_packing( GTK1.Widget child, int side, int anchor, int options, int border_width, int pad_x, int pad_y, int i_pad_x, int i_pad_y );
//! side is one of @[SIDE_BOTTOM], @[SIDE_LEFT], @[SIDE_RIGHT] and @[SIDE_TOP], anchor is one of @[ANCHOR_CENTER], @[ANCHOR_E], @[ANCHOR_EAST], @[ANCHOR_N], @[ANCHOR_NE], @[ANCHOR_NORTH], @[ANCHOR_NORTH_EAST], @[ANCHOR_NORTH_WEST], @[ANCHOR_NW], @[ANCHOR_S], @[ANCHOR_SE], @[ANCHOR_SOUTH], @[ANCHOR_SOUTH_EAST], @[ANCHOR_SOUTH_WEST], @[ANCHOR_SW], @[ANCHOR_W] and @[ANCHOR_WEST],
//! options is a bitwise or of GTK1.PackExpand, GTK1.FillX and GTK1.FillY
//!
//!

GTK1.Packer set_default_border_width( int border );
//!

GTK1.Packer set_default_ipad( int xpad, int ypad );
//!

GTK1.Packer set_default_pad( int xpad, int ypad );
//!

GTK1.Packer set_spacing( int new_spacing );
//!
