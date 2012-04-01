//!

inherit GTK1.Container;

protected GTK1.Layout create( GTK1.Adjustment hadjustment, GTK1.Adjustment vadjustment );
//!

GTK1.Layout freeze( );
//!

GTK1.Adjustment get_hadjustment( );
//!

int get_height( );
//!

GTK1.Adjustment get_vadjustment( );
//!

int get_width( );
//!

int get_xoffset( );
//!

int get_yoffset( );
//!

GTK1.Layout move( GTK1.Widget widget, int x, int y );
//!

GTK1.Layout put( GTK1.Widget widget, int x, int y );
//!

GTK1.Layout set_hadjustment( GTK1.Adjustment adj );
//!

GTK1.Layout set_size( int xsize, int ysize );
//!

GTK1.Layout set_vadjustment( GTK1.Adjustment adj );
//!

GTK1.Layout thaw( );
//!
