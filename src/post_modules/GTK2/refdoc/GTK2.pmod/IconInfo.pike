//!

GTK2.IconInfo copy( );
//! Make a copy.
//!
//!

GTK2.IconInfo destroy( );
//! Destructor.
//!
//!

int get_base_size( );
//! Gets the base size for the icon.  The base size is a size for the icon that
//! was specified by the icon theme creator.  This may be different than the
//! actual size of image; an example of this is small emblem icons that can be
//! attached to a larger icon.  These icons will be given the same base size
//! as the larger icons to which they are attached.
//!
//!

GTK2.GdkPixbuf get_builtin_pixbuf( );
//! Gets the built-in image for this icon, if any.  To allow GTK2+ to use
//! built-in icon images, you must pass the GTK2.ICON_LOOKUP_USE_BUILTIN to
//! GTK2.IconTheme->lookup_icon().
//!
//!

string get_filename( );
//! Gets the filename for the icon.  If the GTK2.ICON_LOOKUP_USE_BUILTIN flag
//! was passed to GTK2.IconTheme->lookup_icon(), there may be no filename if a
//! builtin icon is returned; in this case, you should use get_builtin_pixbuf().
//!
//!

GTK2.GdkPixbuf load_icon( );
//! Renders an icon previously looked up in an icon theme using 
//! GTK2.IconTheme->lookup_icon(); the size will be based on the size passed to
//! GTK2.IconTheme->lookup_icon().  Note that the resulting pixbuf may not be
//! exactly this size; an icon theme may have icons that differe slightly from
//! their nominal sizes, and in addition GTK2+ will avoid scaling icons that it
//! considers sufficiently close to the requested size or for which the source
//! image would have to be scaled up too far. (This maintains sharpness.)
//!
//!
