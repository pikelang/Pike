//! GtkDatabox is designed to display large amounts of numerical data
//! fast and easy. Thousands of data points (X and Y coordinate) may be
//! displayed without any problems, zooming and scrolling as well as
//! optional rulers are already included.
//! 
//! The widget may be used as display for oscilloscopes or other
//! applications that need to display fast changes in their data.
//! 
//!@expr{   GTK2.Databox x=GTK2.Databox(); x->data_add_x_y(3, ({ 1.0, 0.5, 0.0 }), ({1.0, -1.0, 0.0}),GTK2.GdkColor(Image.Color.red), GTK2.DataboxLines,2); x->rescale(); x->set_size_request(300,300); return x;@}
//!@xml{<image>../images/gtk2_databox.png</image>@}
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

inherit GTK2.Vbox;

static GTK2.Databox create( );
//! Create a new databox widget
//!
//!

int data_add_x( int nelems, array x, int shared_Y_index, GTK2.GdkColor color, int type, int dot_size );
//! Type is one of @[DATABOX_BARS], @[DATABOX_CROSS_SIMPLE], @[DATABOX_GRID], @[DATABOX_LINES], @[DATABOX_NOT_DISPLAYED] and @[DATABOX_POINTS]
//!
//!

int data_add_x_y( int nelems, array x, array y, GTK2.GdkColor color, int type, int dot_size );
//! Type is one of @[DATABOX_BARS], @[DATABOX_CROSS_SIMPLE], @[DATABOX_GRID], @[DATABOX_LINES], @[DATABOX_NOT_DISPLAYED] and @[DATABOX_POINTS]
//!
//!

int data_add_y( int nelems, array y, int shared_X_index, GTK2.GdkColor color, int type, int dot_size );
//! Type is one of @[DATABOX_BARS], @[DATABOX_CROSS_SIMPLE], @[DATABOX_GRID], @[DATABOX_LINES], @[DATABOX_NOT_DISPLAYED] and @[DATABOX_POINTS]
//!
//!

GTK2.GdkColor data_get_color( int index );
//! Get the color at index.
//!
//!

array data_get_extrema( );
//!

int data_get_grid_config( int index );
//! See data_set_grid_config().
//!
//!

array data_get_type( int index );
//! Return type and dot size at index.
//!
//!

mapping data_get_value( int x, int y );
//!

array data_get_visible_extrema( );
//!

int data_remove( int index );
//! Remove data at index.
//!
//!

int data_remove_all( );
//! Remove all data.
//!
//!

int data_set_color( int index, GTK2.GdkColor color );
//!

int data_set_grid_config( int index, int hlines, int vlines );
//! Not useful unless the data type is GTK2.DATABOX_GRID.
//!
//!

int data_set_type( int index, int type, int dot_size );
//! Type is one of @[DATABOX_BARS], @[DATABOX_CROSS_SIMPLE], @[DATABOX_GRID], @[DATABOX_LINES], @[DATABOX_NOT_DISPLAYED] and @[DATABOX_POINTS]
//!
//!

GTK2.Databox get_rulers_enable( );
//! Get whether rulers will be displayed.
//!
//!

GTK2.Databox get_scrollbars_enable( );
//! Get whether scrollbars will be displayed.
//!
//!

int get_selection_enable( );
//! Get whether selection is enabled.
//!
//!

int get_selection_fill_enable( );
//! Get whether the selection will be filled.
//!
//!

GTK2.Databox get_zoom_enable( );
//! Get whether zoom is enabled.
//!
//!

float get_zoom_limit( );
//! Get the zoom limit.
//!
//!

GTK2.Databox redraw( );
//! Redraw.
//!
//!

GTK2.Databox rescale( );
//!

GTK2.Databox rescale_inverted( int invertX, int invertY );
//! Rescale with an inverted x and/or y direction.
//!
//!

GTK2.Databox rescale_with_values( float minx, float miny, float maxx, float maxy );
//!

GTK2.Databox set_background_color( GTK2.GdkColor color );
//! Set the background color.
//!
//!

GTK2.Databox set_rulers_enable( int setting );
//! Set whether rulers will be displayed.
//!
//!

GTK2.Databox set_scrollbars_enable( int setting );
//! Set whether scrollbars will be displayed.
//!
//!

GTK2.Databox set_selection_enable( int setting );
//! Set whether selection is enabled.
//!
//!

GTK2.Databox set_selection_fill_enable( int setting );
//! Set whether the selection will be filled.
//!
//!

GTK2.Databox set_zoom_enable( int setting );
//! Set whether zoom is enabled.
//!
//!

GTK2.Databox set_zoom_limit( float limit );
//! Set the zoom limit.  Default is 0.01, which is 100 times.
//!
//!
