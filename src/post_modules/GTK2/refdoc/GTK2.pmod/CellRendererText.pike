//! Properties:
//! Pango.AttrList attributes
//! string background
//! GDK2.Color background-gdk
//! int background-set
//! int editable
//! int editable-set
//! int ellipsize 
//! int ellipsize-set
//! string family
//! int family-set
//! string font
//! Pango.FontDescription font-desc
//! string foreground
//! GDK2.Color foreground-gdk
//! int foreground-set
//! string language
//! int language-set
//! string markup
//! int rise
//! int rise-set
//! float scale
//! int scale-set
//! int single-paragraph-mode
//! int size
//! float size-points
//! int size-set
//! int stretch @[PANGO_STRETCH_CONDENSED], @[PANGO_STRETCH_EXPANDED], @[PANGO_STRETCH_EXTRA_CONDENSED], @[PANGO_STRETCH_EXTRA_EXPANDED], @[PANGO_STRETCH_NORMAL], @[PANGO_STRETCH_SEMI_CONDENSED], @[PANGO_STRETCH_SEMI_EXPANDED], @[PANGO_STRETCH_ULTRA_CONDENSED] and @[PANGO_STRETCH_ULTRA_EXPANDED]
//! int stretch-set
//! int strikethrough
//! int strikethrough-set
//! int style @[PANGO_STYLE_ITALIC], @[PANGO_STYLE_NORMAL] and @[PANGO_STYLE_OBLIQUE]
//! int style-set
//! string text
//! int underline @[PANGO_UNDERLINE_DOUBLE], @[PANGO_UNDERLINE_ERROR], @[PANGO_UNDERLINE_LOW], @[PANGO_UNDERLINE_NONE] and @[PANGO_UNDERLINE_SINGLE]
//! int underline-set
//! int variant @[PANGO_VARIANT_NORMAL] and @[PANGO_VARIANT_SMALL_CAPS]
//! int variant-set
//! int weight
//! int weight-set
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
