//!

inherit Container;

static Layout create( GTK.Adjustment hadjustment, GTK.Adjustment vadjustment );
//!

Layout freeze( );
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

Layout move( GTK.Widget widget, int x, int y );
//!

Layout put( GTK.Widget widget, int x, int y );
//!

Layout set_hadjustment( GTK.Adjustment adj );
//!

Layout set_size( int xsize, int ysize );
//!

Layout set_vadjustment( GTK.Adjustment adj );
//!

Layout thaw( );
//!
