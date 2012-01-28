//! This class creates a GDK1.Pixmap from either an GDK1.Image or
//! Image.Image object (or a numeric ID, see your X-manual for XIDs).
//!  The GDK1.Pixmap object can be used in a lot
//! of different GTK widgets.  The most notable is the W(Pixmap)
//! widget.
//!
//! NOIMG
//!
//!

inherit GDK1.Drawable;

protected GDK1.Pixmap create( int|object image );
//! Create a new GDK1.Pixmap object.
//! Argument is a GDK1.Image object or a Image.Image object
//!
//!

GDK1.Pixmap destroy( );
//! Destructor. Destroys the pixmap.
//!
//!

GDK1.Pixmap ref( );
//!

GDK1.Pixmap set( GDK1.Image image );
//! Argument is a GDK1.Image object or an Image.Image object.
//! It is much faster to use an gdkImage object, especially one
//! allocated in shared memory. This is only an issue if you are
//! going to change the contents of the pixmap often, toggling between
//! a small number of images.
//!
//!

GDK1.Pixmap unref( );
//!
