// Automatically generated from "gtktexttag.pre".
// Do NOT edit.

//! Properties:
//! string background
//! int background-full-height
//! int background-full-height-set
//! GDK2.Color background-gdk
//! int background-set
//! GDK2.Pixmap background-stipple
//! int background-stipple-set
//! int direction
//!   One of @[TEXT_DIR_LTR], @[TEXT_DIR_NONE] and @[TEXT_DIR_RTL]
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
//!   One of @[JUSTIFY_CENTER], @[JUSTIFY_FILL], @[JUSTIFY_LEFT] and @[JUSTIFY_RIGHT]
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
//!   One of @[WRAP_CHAR], @[WRAP_NONE], @[WRAP_WORD] and @[WRAP_WORD_CHAR]
//! int wrap-mode-set;
//!
//!
//!  Signals:
//! @b{event@}
//!

inherit G.Object;
//!

protected void create( void name_or_props );
//! Creates a new text tag.
//!
//!

int event( G.Object event_object, GDK2.Event event, GTK2.TextIter iter );
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
