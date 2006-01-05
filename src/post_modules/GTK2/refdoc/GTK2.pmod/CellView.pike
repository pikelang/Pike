//! Properties:
//! string background
//! GDK2.Color background-gdk
//! int background-set
//!
//!

inherit GTK2.Widget;

inherit GTK2.CellLayout;

static GTK2.CellView create( string|GdkPixbuf|mapping text, int|void markup );
//! Create a new W(CellView) widget.
//!
//!

GTK2.TreePath get_displayed_row( );
//! Returns a W(TreePath) referring to the currently displayed row.  If no
//! row is currently displayed, 0 is returned.
//!
//!

mapping get_size_of_row( GTK2.TreePath path );
//! Returns width and height of the size needed to display the model row
//! pointed to by path.
//!
//!

GTK2.CellView set_background_color( GTK2.GdkColor color );
//! Sets the background color.
//!
//!

GTK2.CellView set_displayed_row( GTK2.TreePath path );
//! Sets the row of the model that is currently displayed.  If the path is
//! omitted, then the contents of the cellview "stick" at their last value;
//! this is not normally a desired result, but may be a needed intermediate
//! state if say, the mode becomes temporarily empty.
//!
//!

GTK2.CellView set_model( GTK2.TreeModel model );
//! Sets the model.  If this view already has a model set, it will remove it
//! before setting the new model.
//!
//!
