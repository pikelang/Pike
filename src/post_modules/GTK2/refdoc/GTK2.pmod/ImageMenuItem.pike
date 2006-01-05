//! Properties:
//! GTK2.Widget image
//!
//!

inherit GTK2.MenuItem;

static GTK2.ImageMenuItem create( string|mapping label );
//! Create a new ImageMenuItem.
//!
//!

GTK2.Widget get_image( );
//! Gets the widget that is currently set as the image.
//!
//!

GTK2.ImageMenuItem set_image( GTK2.Widget image );
//! Sets the image of the image menu item.
//!
//!
