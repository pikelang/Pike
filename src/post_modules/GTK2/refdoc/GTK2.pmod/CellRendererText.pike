//! Properties:
//! string background
//! GDK2.Color background-gdk
//! int background-set
//! int editable
//! int editable-set
//! string foreground
//! GDK2.Color foreground-gdk
//! int foreground-set
//! string language
//! int language-set
//! int rise
//! int rise-set
//! float scale
//! int scale-set
//! int single-paragraph-mode
//! int size
//! float size-points
//! int size-set
//! int strikethrough
//! int strikethrough-set
//! string text
//! int width-chars
//!
//!
//!  Signals:
//! @b{edited@}
//!

inherit GTK2.CellRenderer;

static GTK2.CellRendererText create( mapping|void props );
//! Creates a new W(CellRendererText).  Adjust how text is drawn using object
//! properties.  Object properties can be set globally (with G.Object->set()).
//! Also, with W(TreeViewColumn), you can bind a property to a value in a
//! W(TreeModel).  For example, you can bind the "text" property on the cell
//! renderer to a string value in the model, thus rendering a different string
//! in each row of the W(TreeView).
//!
//!

GTK2.CellRendererText set_fixed_height_from_font( int number_of_rows );
//! Sets the height of a renderer to explicitly be determined by the "font"
//! and "y_bad" property set on it.  Further changes in these properties do not
//! affect the height, so they must be accompanied by a subsequent call to
//! this function.  Using this function is unflexible, and should really only
//! be used if calculating the size of cell is too slow (i.e., a massive
//! number of cells displayed).  If number_of_rows is -1, then the fixed height
//! is unset, and the height is determined by the properties again.
//!
//!
