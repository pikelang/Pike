//! The Gtk.Table allow the programmer to arrange widgets in rows and
//! columns, making it easy to align many widgets next to each other,
//! horizontally and vertically.
//! 
//!@expr{ GTK1.Table(2,2,0)->attach_defaults( GTK1.Label("0,0"), 0, 1, 0, 1)->attach_defaults( GTK1.Label("0,1"), 0, 1, 1, 2)->attach_defaults( GTK1.Label("1,0"), 1, 2, 0, 1)->attach_defaults( GTK1.Label("1,1"), 1, 2, 1, 2)->set_col_spacings(10)->set_row_spacings(10)@}
//!@xml{<image>../images/gtk1_table.png</image>@}
//!
//!@expr{ GTK1.Table(2,2,0)->attach_defaults( GTK1.Label("0,0-1,0"), 0, 2, 0, 1)->attach_defaults( GTK1.Label("0,1"), 0, 1, 1, 2)->attach_defaults( GTK1.Label("1,1"), 1, 2, 1, 2)->set_col_spacings(10)->set_row_spacings(10)@}
//!@xml{<image>../images/gtk1_table_2.png</image>@}
//!
//! 
//!
//!

inherit GTK1.Container;

GTK1.Table attach( GTK1.Widget subwidget, int left, int right, int top, int bottom, int xoptions, int yoptions, int xpad, int ypad );
//! The left and right attach arguments specify where to place the
//! widget, and how many boxes to use. If you want a button in the
//! lower right table entry of our 2x2 table, and want it to fill that
//! entry ONLY. left_attach would be = 1, right_attach = 2, top_attach
//! = 1, bottom_attach = 2.
//! 
//! Now, if you wanted a widget to take up the whole top row of our 2x2
//! table, you'd use left_attach = 0, right_attach = 2, top_attach = 0,
//! bottom_attach = 1.
//! 
//! The xoptions and yoptions are used to specify packing options and
//! may be OR'ed together to allow multiple options.
//! 
//! These options are:
//! <ul>
//! <li> GTK1.Fill - If the table box is larger than the widget, and
//! GTK_FILL is specified, the widget will expand to use all the room
//! available.</li>
//! <li> GTK1.Shrink - If the table widget was allocated less space then
//! was requested (usually by the user resizing the window), then the
//! widgets would normally just be pushed off the bottom of the window
//! and disappear. If GTK_SHRINK is specified, the widgets will shrink
//! with the table.</li>
//! <li> GTK1.Expand - This will cause the table cell to expand to use
//! up any remaining space in the window.</li>
//!  </ul>
//! 
//! Padding is just like in boxes, creating a clear area around the
//! widget specified in pixels
//!
//!

GTK1.Table attach_defaults( GTK1.Widget subwidget, int left, int right, int top, int bottom );
//! As there are many options associated with attach(), this
//! convenience function provides the programmer with a means to add
//! children to a table with identical padding and expansion options.
//! 
//! xoptions, yoptions, xpadding and ypadding are all set the their
//! default values. For the options that is GTK1.Fill|GTK1.Expand. For
//! the padding it is 0.
//!
//!

protected GTK1.Table create( int width, int height, int homogeneousp );
//! Used to create a new table widget. An initial size must be given by
//! specifying how many rows and columns the table should have,
//! although this can be changed later with resize().
//! 
//! There can never be more than 65535 colums nor more than 65535 rows.
//! 
//! setting homogeneousp to 1 forces the all tablecells to be exactly
//! the same size.
//!
//!

int get_column_spacing( );
//! The spacing between all columns, in pixels.
//!
//!

int get_homogeneous( );
//! If 1, all cells have exactly the same size
//!
//!

int get_ncols( );
//! The number of columns. Between 1 and 65535.
//!
//!

int get_nrows( );
//! The number of rows. Between 1 and 65535.
//!
//!

int get_row_spacing( );
//! The spacing between all rows, in pixels.
//!
//!

GTK1.Table resize( int rows, int columns );
//! If you need to change a table's size after it has been created,
//! this function allows you to do so.
//!
//!

GTK1.Table set_col_spacing( int x, int spacing );
//! alters the amount of space between a given table column and the
//! adjacent columns.
//!
//!

GTK1.Table set_col_spacings( int spacing );
//! Sets the space between every column in the table equal to spacing.
//!
//!

GTK1.Table set_homogeneous( int homogeneousp );
//! Set the homogeneous flag.
//!
//!

GTK1.Table set_row_spacing( int y, int spacing );
//! alters the amount of space between a given table row and the
//! adjacent rows.
//!
//!

GTK1.Table set_row_spacings( int spacing );
//! Sets the space between every row in the table equal to spacing.
//!
//!
