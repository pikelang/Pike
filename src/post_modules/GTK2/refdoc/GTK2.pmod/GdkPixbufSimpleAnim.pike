//! Properties:
//! bool loop
//!
//!

inherit GDK2.PixbufAnimation;

GDK2.PixbufSimpleAnim add_frame( GTK2.GdkPixbuf frame );
//! Adds a new frame to animation . The pixbuf must have the
//! dimensions specified when the animation was constructed.
//!
//!

protected GDK2.PixbufSimpleAnim create( int width, int height, float rate );
// Create an empty animation.
// @[rate] is the frames per second, width and height in pixels.
//

int(0..1) get_loop( );
//! Returns if animation will loop indefinitely when it reaches
//! the end.
//!
//!

GDK2.PixbufSimpleAnim set_loop( int(0..1) loop );
//! Sets whether animation should loop indefinitely when it reaches
//! the end.
//!
//!
