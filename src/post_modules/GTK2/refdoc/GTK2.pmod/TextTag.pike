//! Properties:
//! string background
//! int background-full-height
//! int background-full-height-set
//! GDK2.Color background-gdk
//! int background-set
//! GDK2.Pixmap background-stipple
//! int background-stipple-set
//! int direction
//!   One of 
//! int editable
//! int editable-set
//! string family
//! int family-set
//! string font
//! Pango.FontDescription font-desc
//! string foreground
//! GDK2.Color foreground-gdk
//! int foreground-set
//! GDK2.Pixmap foreground-stipple
//! int foreground-stipple-set
//! int indent
//! int indent-set
//! int invisible
//! int invisible_set
//! int justification
//!   One of 
//! int justification-set
//! string language;
//! int language-set;
//! int left-margin;
//! int left-margin-set;
//! string name;
//! int pixels-above-lines;
//! int pixels-above-lines-set;
//! int pixels-below-lines;
//! int pixels-below-lines-set;
//! int pixels-inside-wrap;
//! int pixels-inside-wrap-set;
//! int right-margin;
//! int right-margin-set;
//! int rise;
//! int rise-set;
//! float scale;
//! int scale-set;
//! int size;
//! double size-points;
//! int size-set;
//! int stretch;
//!   One of @[PANGO_STRETCH_CONDENSED], @[PANGO_STRETCH_EXPANDED], @[PANGO_STRETCH_EXTRA_CONDENSED], @[PANGO_STRETCH_EXTRA_EXPANDED], @[PANGO_STRETCH_NORMAL], @[PANGO_STRETCH_SEMI_CONDENSED], @[PANGO_STRETCH_SEMI_EXPANDED], @[PANGO_STRETCH_ULTRA_CONDENSED] and @[PANGO_STRETCH_ULTRA_EXPANDED]
//! int stretch-set;
//! int strikethrough;
//! int strikethrough-set;
//! int style;
//!   One of @[PANGO_STYLE_ITALIC], @[PANGO_STYLE_NORMAL] and @[PANGO_STYLE_OBLIQUE]
//! int style-set;
//! Pango.TabArray tabs
//! int tabs-set;
//! int underline;
//!   One of @[PANGO_UNDERLINE_DOUBLE], @[PANGO_UNDERLINE_ERROR], @[PANGO_UNDERLINE_LOW], @[PANGO_UNDERLINE_NONE] and @[PANGO_UNDERLINE_SINGLE]
//! int underline-set;
//! int variant;
//!   One of @[PANGO_VARIANT_NORMAL] and @[PANGO_VARIANT_SMALL_CAPS]
//! int variant-set;
//! int weight;
//! int weight-set;
//! int wrap-mode;
//!   One of 
//! int wrap-mode-set;
//!
//!
//!  Signals:
//! @b{event@}
//!

inherit G.Object;

static GTK2.TextTag create( string|mapping name_or_props );
//! Creates a new text tag.
//!
//!

int event( GTK2.G.Object event_object, GTK2.GdkEvent event, GTK2.TextIter iter );
//! Emits the 'event' signal.
//!
//!

int get_priority( );
//! Gets the tag priority.
//!
//!

GTK2.TextTag set_priority( int priority );
//! Sets the priority.  Valid priorities start at 0 and go to 1 less
//! than W(TextTagTable)->get_size().
//!
//!
