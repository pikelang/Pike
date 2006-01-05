//! Properties:
//! string cell-background
//! GDK2.Color cell-background-gdk
//! int cell-background-set
//! int height
//! int is-expanded
//! int is-expander
//! int mode @[CELL_RENDERER_MODE_ACTIVATABLE], @[CELL_RENDERER_MODE_EDITABLE] and @[CELL_RENDERER_MODE_INERT]
//! int sensitive
//! int visible
//! int width
//! float xalign
//! int xpad
//! float yalign
//! int ypad
//!
//!
//!  Signals:
//! @b{editing_canceled@}
//!
//! @b{editing_started@}
//!

inherit GTK2.Data;

int activate( GTK2.GdkEvent event, GTK2.Widget widget, string path, GTK2.GdkRectangle background_area, GTK2.GdkRectangle cell_area, int flags );
//! Passes an activate event to the cell renderer for possible processing.
//! Some cell renderers may use events; for example, W(CellRendererToggle)
//! toggles when it gets a mouse click.
//!
//!

mapping get_fixed_size( );
//! Fills in width and height with the appropriate size of the cell.
//!
//!

mapping get_size( GTK2.Widget widget, GTK2.GdkRectangle cell_area );
//! Obtains the width and heigh needed to render the cell.  Used by widgets
//! to determine the appropriate size for the cell_area passed to render().
//! If cell_area is present, fills in the x and y offset of the cell relative
//! to this location.  Please note that the values set in width and height,
//! as well as those in x_offset and y_offset are inclusive of the xpad and
//! ypad properties.
//!
//!

GTK2.CellRenderer render( GTK2.GdkWindow window, GTK2.Widget widget, GTK2.GdkRectangle background_area, GTK2.GdkRectangle cell_area, GTK2.GdkRectangle expose_area, int flags );
//! Invokes the virtual render function of the W(CellRenderer).  The three
//! passed-in rectangles are areas of window.  Most renderers will draw within
//! cell_area; the xalign, yalign, xpad, and ypad fields of the W(CellRenderer)
//! should be honored with respect to cell_area.  background_area includes the
//! blank space around the cell, and also the area containing the tree
//! expander; so the background_area rectangles for all cells tile to cover the
//! entire window.  expose_area is a clip rectangle.
//! flags is one of @[CELL_RENDERER_FOCUSED], @[CELL_RENDERER_INSENSITIVE], @[CELL_RENDERER_MODE_ACTIVATABLE], @[CELL_RENDERER_MODE_EDITABLE], @[CELL_RENDERER_MODE_INERT], @[CELL_RENDERER_PRELIT], @[CELL_RENDERER_SELECTED] and @[CELL_RENDERER_SORTED].
//!
//!

GTK2.CellRenderer set_fixed_size( int width, int height );
//! Sets the renderer size to be explicit, independent of the properties set.
//!
//!

GTK2.CellRenderer start_editing( GTK2.GdkEvent event, GTK2.Widget widget, string path, GTK2.GdkRectangle background_area, GTK2.GdkRectangle cell_area, int flags );
//! Passes an activate event to the cell renderer for possible processing.
//!
//!

GTK2.CellRenderer stop_editing( int canceled );
//! Informs the cell renderer that the editing is stopped.  If canceled is
//! true, the cell renderer will emit the "editing-canceled" signal.  This
//! function should be called by cell renderer implementations in response to
//! the "editing-done" signal of W(CellEditable).
//!
//!
