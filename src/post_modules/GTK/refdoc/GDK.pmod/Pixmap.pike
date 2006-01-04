//! This class creates a GDK.Pixmap from either an GDK.Image or
//! Image.Image object (or a numeric ID, see your X-manual for XIDs).
//!  The GDK.Pixmap object can be used in a lot
//! of different GTK widgets.  The most notable is the W(Pixmap)
//! widget.
//!
//! NOIMG
//!
//!

inherit GDK.Drawable;

static GDK.Pixmap create( int|object image );
//! Create a new GDK.Pixmap object.
//! Argument is a GDK.Image object or a Image.Image object
//!
//!

GDK.Pixmap destroy( );
//! Destructor. Destroys the pixmap.
//!
//!

GDK.Pixmap ref( );
//!

GDK.Pixmap set( GDK.Image image );
//! Argument is a GDK.Image object or an Image.Image object.
//! It is much faster to use an gdkImage object, especially one
//! allocated in shared memory. This is only an issue if you are
//! going to change the contents of the pixmap often, toggling between
//! a small number of images.
//!
//!

GDK.Pixmap unref( );
//!
