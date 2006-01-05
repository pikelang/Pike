//! The GTK2.Table allows the programmer to arrange widgets in rows and
//! columns, making it easy to align many widgets next to each other,
//! horizontally and vertically.
//! 
//!@expr{ GTK2.Table(2,2,0)->attach_defaults( GTK2.Label("0,0"), 0, 1, 0, 1)->attach_defaults( GTK2.Label("0,1"), 0, 1, 1, 2)->attach_defaults( GTK2.Label("1,0"), 1, 2, 0, 1)->attach_defaults( GTK2.Label("1,1"), 1, 2, 1, 2)->set_col_spacings(10)->set_row_spacings(10)@}
//!@xml{<image>../images/gtk2_table.png</image>@}
//!
//!@expr{ GTK2.Table(2,2,0)->attach_defaults( GTK2.Label("0,0-1,0"), 0, 2, 0, 1)->attach_defaults( GTK2.Label("0,1"), 0, 1, 1, 2)->attach_defaults( GTK2.Label("1,1"), 1, 2, 1, 2)->set_col_spacings(10)->set_row_spacings(10)@}
//!@xml{<image>../images/gtk2_table_2.png</image>@}
//!
//! 
//! Properties:
//! int column-spacing
//! int homogeneous
//! int n-columns
//! int n-rows
//! int row-spacing
//! 
//! Child properties:
//! int bottom-attach
//! int left-attach
//! int right-attach
//! int top-attach
//! int x-options
//! int x-padding
//! int y-options
//! int y-padding
//!
//!

inherit GTK2.Container;

GTK2.Table attach( GTK2.Widget subwidget, int left, int right, int top, int bottom, int xoptions, int yoptions, int xpad, int ypad );
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
//! <li> GTK2.Fill - If the table box is larger than the widget, and
//! GTK_FILL is specified, the widget will expand to use all the room
//! available.</li>
//! <li> GTK2.Shrink - If the table widget was allocated less space then
//! was requested (usually by the user resizing the window), then the
//! widgets would normally just be pushed off the bottom of the window
//! and disappear. If GTK_SHRINK is specified, the widgets will shrink
//! with the table.</li>
//! <li> GTK2.Expand - This will cause the table cell to expand to use
//! up any remaining space in the window.</li>
//!  </ul>
//! 
//! Padding is just like in boxes, creating a clear area around the
//! widget specified in pixels
//!
//!

GTK2.Table attach_defaults( GTK2.Widget subwidget, int left, int right, int top, int bottom );
//! As there are many options associated with attach(), this
//! convenience function provides the programmer with a means to add
//! children to a table with identical padding and expansion options.
//! 
//! xoptions, yoptions, xpadding and ypadding are all set the their
//! default values. For the options that is GTK2.Fill|GTK2.Expand. For
//! the padding it is 0.
//!
//!

static GTK2.Table create( int|mapping width_or_props, int|void height, int|void homogeneousp );
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

int get_col_spacing( int col );
//! Gets the amound of space between col and col+1.
//!
//!

int get_default_col_spacing( );
//! Gets the default column spacing for the table.
//!
//!

int get_default_row_spacing( );
//! Gets the default row spacing for the table.
//!
//!

int get_homogeneous( );
//! Returns whether the table cells are all constrained to the same
//! width and height.
//!
//!

int get_row_spacing( int row );
//! Gets the amount of space between row and row+1.
//!
//!

GTK2.Table resize( int rows, int columns );
//! If you need to change a table's size after it has been created,
//! this function allows you to do so.
//!
//!

GTK2.Table set_col_spacing( int x, int spacing );
//! alters the amount of space between a given table column and the
//! adjacent columns.
//!
//!

GTK2.Table set_col_spacings( int spacing );
//! Sets the space between every column in the table equal to spacing.
//!
//!

GTK2.Table set_homogeneous( int homogeneousp );
//! Set the homogeneous flag.
//!
//!

GTK2.Table set_row_spacing( int y, int spacing );
//! alters the amount of space between a given table row and the
//! adjacent rows.
//!
//!

GTK2.Table set_row_spacings( int spacing );
//! Sets the space between every row in the table equal to spacing.
//!
//!
