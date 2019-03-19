//!
//!
//!

inherit G.Object;

GTK2.Pango.Context create_pango_context( );
//! Creates a new Pango.Context that can be used with this PrintContext.
//!
//!

GTK2.Pango.Layout create_pango_layout( );
//! Creates a new Pango.Layout that is suitable for use with this PrintContext.
//!
//!

float get_dpi_x( );
//! Obtains the horizontal resolution, in dots per inch.
//!
//!

float get_dpi_y( );
//! Obtains the vertical resolution, in dots per inch.
//!
//!

float get_height( );
//! Obtains the height, in pixels.
//!
//!

GTK2.PageSetup get_page_setup( );
//! Obtains the GTK2.PageSetup that determines the page dimensions.
//!
//!

float get_width( );
//! Obtains the width, in pixels.
//!
//!
