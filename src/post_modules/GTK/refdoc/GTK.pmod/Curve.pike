//!@code{ GTK.Curve()@}
//!@xml{<image src='../images/gtk_curve.png'/>@}
//!
//!
//!
inherit DrawingArea;

static Curve create( )
//!

array(float) get_vector( int num_points )
//!

Curve reset( )
//!

Curve set_curve_type( int type )
//! One of @[CURVE_TYPE_SPLINE], @[CURVE_TYPE_LINEAR] and @[CURVE_TYPE_FREE];
//!
//!

Curve set_gamma( float gamma )
//!

Curve set_range( float min_x, float max_x, float min_y, float max_y )
//!

Curve set_vector( int nelems, array(float) curve )
//!
