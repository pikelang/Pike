//! GtkDatabox is designed to display large amounts of numerical data
//! fast and easy. Thousands of data points (X and Y coordinate) may be
//! displayed without any problems, zooming and scrolling as well as
//! optional rulers are already included.
//! 
//! The widget may be used as display for oscilloscopes or other
//! applications that need to display fast changes in their data.
//! 
//!@code{   GTK.Databox x=GTK.Databox(); x->data_add_x_y(3, ({ 1.0, 0.5, 0.0 }), ({1.0, -1.0, 0.0}),GDK.Color(Image.Color.red), GTK.DataboxLines,2); x->rescale(); x->set_usize(300,300); return x;@}
//!@xml{<image>../images/gtk_databox.png</image>@}
//!
//!
//!
//!  Signals:
//! @b{marked@}
//!
//! @b{selection_canceled@}
//!
//! @b{selection_changed@}
//!
//! @b{selection_started@}
//!
//! @b{selection_stopped@}
//!
//! @b{zoomed@}
//!

inherit GTK.Vbox;

static GTK.Databox create( );
//! Create a new databox widget
//!
//!

int data_add_x( int nelems, array x, int shared_Y_index, GDK.Color color, int type, int dot_size );
//! Type is one of @[DATABOX_BARS], @[DATABOX_LINES], @[DATABOX_NOT_DISPLAYED] and @[DATABOX_POINTS]
//!
//!

int data_add_x_y( int nelems, array x, array y, GDK.Color color, int type, int dot_size );
//! Type is one of @[DATABOX_BARS], @[DATABOX_LINES], @[DATABOX_NOT_DISPLAYED] and @[DATABOX_POINTS]
//!
//!

int data_add_y( int nelems, array y, int shared_X_index, GDK.Color color, int type, int dot_size );
//! Type is one of @[DATABOX_BARS], @[DATABOX_LINES], @[DATABOX_NOT_DISPLAYED] and @[DATABOX_POINTS]
//!
//!

int data_destroy( int index );
//!

int data_destroy_all( );
//!

array data_get_extrema( );
//!

mapping data_get_value( int x, int y );
//!

array data_get_visible_extrema( );
//!

GTK.Databox disable_zoom( );
//!

GTK.Databox enable_zoom( );
//!

GTK.Databox hide_cross( );
//!

GTK.Databox hide_rulers( );
//!

GTK.Databox hide_scrollbars( );
//!

GTK.Databox rescale( );
//!

GTK.Databox rescale_with_values( float minx, float miny, float maxx, float maxy );
//!

int set_color( int index, GDK.Color color );
//!

int set_data_type( int index, int type, int dot_size );
//! Type is one of @[DATABOX_BARS], @[DATABOX_LINES], @[DATABOX_NOT_DISPLAYED] and @[DATABOX_POINTS]
//!
//!

GTK.Databox show_cross( );
//!

GTK.Databox show_rulers( );
//!

GTK.Databox show_scrollbars( );
//!
