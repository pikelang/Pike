//! Application-driven progressive image loading.
//!
//!
//!  Signals:
//! @b{area_prepared@}
//!
//! @b{area_updated@}
//!
//! @b{closed@}
//!
//! @b{size_prepared@}
//!

inherit G.Object;

int(0..1) close( );
//! informs a pixbuf loader that no further writes with write() will
//! occur, so that it can free its internal loading structures. Also,
//! tries to parse any data that hasn't yet been parsed; if the
//! remaining data is partial or corrupt, FALSE will be returned.
//!
//!

protected GDK2.PixbufLoader create( );
//!
//!
//!

GTK2.GdkPixbufAnimation get_animation( );
//! Queries the GDK2.PixbufAnimation that a pixbuf loader is currently
//! creating. In general it only makes sense to call this function
//! after the "area-prepared" signal has been emitted by the
//! loader. If the loader doesn't have enough bytes yet (hasn't
//! emitted the "area-prepared" signal) this function will return
//! NULL.
//!
//!

GTK2.GdkPixbuf get_pixbuf( );
// Queries the GdkPixbuf that a pixbuf loader is currently
// creating. In general it only makes sense to call this function
// after the "area-prepared" signal has been emitted by the loader;
// this means that enough data has been read to know the size of the
// image that will be allocated.
//
// If the loader has not received
// enough data via @[write](), then this function
// returns NULL. The returned pixbuf will be the same in all future
// calls to the loader, so you can just keep it around. Additionally,
// if the loader is an animation, it will return the "static image"
// of the animation (see @[GDK2.PixbufAnimation.get_static_image]).
//

GDK2.PixbufLoader set_size( int width, int height );
//! Causes the image to be scaled while it is loaded. Attempts to set
//! the desired image size are ignored after the emission of the
//! size-prepared signal. (once loading start)
//!
//!

int(0..1) write( string|Stdio.Buffer data );
//! This will cause a pixbuf loader to parse the more data for
//! an image. It will return TRUE if the data was loaded successfully,
//! and FALSE if an error occurred. In the latter case, the loader
//! will be closed, and will not accept further writes.
//!
//!
