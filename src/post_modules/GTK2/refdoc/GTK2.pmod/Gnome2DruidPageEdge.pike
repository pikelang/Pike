//!

inherit Gnome2.DruidPage;

static Gnome2.DruidPageEdge create( int position, int|void anti_alias );
//! Create a new Gnome2.DruidPageEdge, with optional anti-aliasing.
//!
//!

Gnome2.DruidPageEdge set_bg_color( GTK2.GdkColor color );
//! This will set the background color.
//!
//!

Gnome2.DruidPageEdge set_logo( GTK2.GdkPixbuf logo );
//! Sets a GDK2.Pixbuf as the logo in the top right corner.  If omitted, then no 
//! logo will be displayed.
//!
//!

Gnome2.DruidPageEdge set_logo_bg_color( GTK2.GdkColor color );
//! Sets the color behind the druid page's logo.
//!
//!

Gnome2.DruidPageEdge set_text( string text );
//! Sets the contents of the text portion.
//!
//!

Gnome2.DruidPageEdge set_text_color( GTK2.GdkColor color );
//! Sets the color of the text in the body of the page.
//!
//!

Gnome2.DruidPageEdge set_textbox_color( GTK2.GdkColor color );
//! Sets the color of the background in the main text area of the page.
//!
//!

Gnome2.DruidPageEdge set_title( string title );
//! Sets the contents of the page's title text.
//!
//!

Gnome2.DruidPageEdge set_title_color( GTK2.GdkColor color );
//! Sets the color of the title text.
//!
//!

Gnome2.DruidPageEdge set_top_watermark( GTK2.GdkPixbuf watermark );
//! Sets a GDK2.Pixbuf as the watermark on top of the top strip on the druid.
//! If watermark is omitted, it is reset to the normal color.
//!
//!

Gnome2.DruidPageEdge set_watermark( GTK2.GdkPixbuf watermark );
//! Sets a GDK2.Pixbuf as the watermark on the left strip on the druid.  If
//! watermark is omitted, it is reset to the normal color.
//!
//!
