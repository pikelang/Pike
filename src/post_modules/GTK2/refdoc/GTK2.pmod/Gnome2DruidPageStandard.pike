//! Properties:
//! string background
//! GDK2.Color background-gdk
//! int background-set
//! string contents-background
//! GDK2.Color contents-background-gdk
//! int contents-background-set
//! GDK2.Pixbuf logo
//! string logo-background
//! GDK2.Color logo-background-gdk
//! int logo-background-set
//! string title
//! string title-foreground
//! GDK2.Color title-foreground-gdk
//! int title-foreground-set
//! GDK2.Pixbuf top-watermark
//!
//!

inherit Gnome2.DruidPage;

Gnome2.DruidPageStandard append_item( string question, GTK2.Widget item, string additional_info );
//! Convenience function to add a GTK2.Widget to the vbox.  This function
//! creates a new contents section that has the question text followed by the
//! item widget and then the additional_info text, all stacked vertically from
//! top to bottom.
//! 
//! The item widget could be something like a set of radio checkbuttons
//! requesting a choice from the user.
//!
//!

static Gnome2.DruidPageStandard create( string|void title, GTK2.GdkPixbuf logo, GTK2.GdkPixbuf top_watermark );
//! Construct a new Gnome2.DruidPageStandard.
//!
//!

Gnome2.DruidPageStandard set_background( GTK2.GdkColor color );
//! Sets the background color of the top section.
//!
//!

Gnome2.DruidPageStandard set_contents_background( GTK2.GdkColor color );
//! Sets the color of the main contents section's background.
//!
//!

Gnome2.DruidPageStandard set_logo( GTK2.GdkPixbuf logo );
//! Sets a GDK2.Pixbuf as the logo in the top right corner.  If omitted, then
//! no logo will be displayed.
//!
//!

Gnome2.DruidPageStandard set_logo_background( GTK2.GdkColor color );
//! Sets the background color of the logo.
//!
//!

Gnome2.DruidPageStandard set_title( string title );
//! Sets the title.
//!
//!

Gnome2.DruidPageStandard set_title_foreground( GTK2.GdkColor color );
//! Sets the title text to the specified color.
//!
//!

Gnome2.DruidPageStandard set_top_watermark( GTK2.GdkPixbuf watermark );
//! Sets a GDK2.Pixbuf as the watermark on top of the top strip on the druid.
//! If watermark is omitted, it is reset to the normal color.
//!
//!
