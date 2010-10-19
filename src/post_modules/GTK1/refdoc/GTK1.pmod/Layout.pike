//!

inherit GTK.Container;

static GTK.Layout create( GTK.Adjustment hadjustment, GTK.Adjustment vadjustment );
//!

GTK.Layout freeze( );
//!

GTK.Adjustment get_hadjustment( );
//!

int get_height( );
//!

GTK.Adjustment get_vadjustment( );
//!

int get_width( );
//!

int get_xoffset( );
//!

int get_yoffset( );
//!

GTK.Layout move( GTK.Widget widget, int x, int y );
//!

GTK.Layout put( GTK.Widget widget, int x, int y );
//!

GTK.Layout set_hadjustment( GTK.Adjustment adj );
//!

GTK.Layout set_size( int xsize, int ysize );
//!

GTK.Layout set_vadjustment( GTK.Adjustment adj );
//!

GTK.Layout thaw( );
//!
