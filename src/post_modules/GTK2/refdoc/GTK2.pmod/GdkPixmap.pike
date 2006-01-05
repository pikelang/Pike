//! This class creates a GDK2.Pixmap from either an GDK2.Image or
//! Image.image object (or a numeric ID, see your X-manual for XIDs).
//!  The GDK2.Pixmap object can be used in a lot
//! of different GTK widgets.  The most notable is the W(Pixmap)
//! widget.
//!
//! NOIMG
//!
//!

inherit GDK2.Drawable;

static GDK2.Pixmap create( int|object image );
//! Create a new GDK2.Pixmap object.
//! Argument is a GDK2.Image object or a Image.Image object
//!
//!

GDK2.Pixmap destroy( );
//! Destructor. Destroys the pixmap.
//!
//!

GDK2.Pixmap ref( );
//! Ref this object.
//!
//!

GDK2.Pixmap set( GTK2.GdkImage image );
//! Argument is a GDK2.Image object or an Image.image object.
//! It is much faster to use an gdkImage object, especially one
//! allocated in shared memory. This is only an issue if you are
//! going to change the contents of the pixmap often, toggling between
//! a small number of images.
//!
//!

GDK2.Pixmap unref( );
//! Unref this object.
//!
//!
