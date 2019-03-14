//! An iterator for a PixbufAnimation.
//!
//!

inherit G.Object;

int advance( );
//! Possibly advance an animation to a new frame.
//!
//!

int get_delay_time( );
//! Gets the number of milliseconds the current pixbuf should be displayed, or
//! -1 if the current pixbuf should be displayed forever.
//!
//!

GTK2.GdkPixbuf get_pixbuf( );
//! Gets the current pixbuf which should be displayed; the pixbuf will be the
//! same size as the animation itself (GDK2.PixbufAnimation->get_width(),
//! GDK2.PixbufAnimation->get_height()).  This pixbuf should be displayed for
//! get_delay_time() milliseconds.
//!
//!

int on_currently_loading_frame( );
//! Used to determine how to respond to the area_updated signal on
//! GDK2.PixbufLoader when loading an animation.
//!
//!
