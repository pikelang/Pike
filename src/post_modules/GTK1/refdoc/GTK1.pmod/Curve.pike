//!@expr{ GTK1.Curve()@}
//!@xml{<image>../images/gtk1_curve.png</image>@}
//!
//!
//!

inherit GTK1.DrawingArea;

protected GTK1.Curve create( );
//!

array get_vector( int num_points );
//!

GTK1.Curve reset( );
//!

GTK1.Curve set_curve_type( int type );
//! One of @[CURVE_TYPE_FREE], @[CURVE_TYPE_LINEAR] and @[CURVE_TYPE_SPLINE];
//!
//!

GTK1.Curve set_gamma( float gamma );
//!

GTK1.Curve set_range( float min_x, float max_x, float min_y, float max_y );
//!

GTK1.Curve set_vector( int nelems, array curve );
//!
