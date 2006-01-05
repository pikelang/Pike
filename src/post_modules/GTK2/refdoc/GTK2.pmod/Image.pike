//! An image is a image object stored in client, not X server, memory.
//! A pixmap, on the other hand, is a image object stored in the X-server.
//! See GDK2.Image and GDK2.Pixmap.
//!
//!@expr{ GTK2.Image("tornado_nguyen_big.jpg");@}
//!@xml{<image>../images/gtk2_image.png</image>@}
//!
//!
//! Properties:
//! string file
//! string icon-name
//! icon-set
//! int icon-size
//! GDK2.Image image
//! GDK2.Pixmap mask
//! GDK2.Pixbuf pixbuf
//! GDK2.PixbufAnimation pixbuf-animation
//! int pixel-size
//! GDK2.Pixmap pixmap
//! string stock
//! int storage-type @[IMAGE_ANIMATION], @[IMAGE_EMPTY], @[IMAGE_ICON_NAME], @[IMAGE_ICON_SET], @[IMAGE_IMAGE], @[IMAGE_PIXBUF], @[IMAGE_PIXMAP] and @[IMAGE_STOCK]
//!
//!

inherit GTK2.Misc;

GTK2.Image clear( );
//! Resets the image to be empty.
//!
//!

static GTK2.Image create( string|GdkPixbuf|GdkPixbufAnimation|GdkImage|GdkPixmap|mapping file_or_props, GTK2.GdkBitmap mask_or_size );
//! Create a new W(Image) from either a file or a GDK2.Pixbuf.
//!
//!

GTK2.GdkPixbufAnimation get_animation( );
//! Gets the GDK2.PixbufAnimation being displayed.
//!
//!

mapping get_icon_name( );
//! Gets the icon name and size.
//!
//!

mapping get_image( );
//! Returns ([ "image":GDK2.Image img, "mask":GDK2.Bitmap mask ])
//!
//!

GTK2.GdkPixbuf get_pixbuf( );
//! Gets the GDK2.Pixbuf being displayed.  The storage type of the image must
//! be GTK2.IMAGE_EMPTY or GTK2.IMAGE_PIXBUF).
//!
//!

int get_pixel_size( );
//! Gets the pixel size used for named icons.
//!
//!

mapping get_pixmap( );
//! Gets the pixmap and mask.
//!
//!

mapping get_stock( );
//! Gets the stock icon name and size.
//!
//!

int get_storage_type( );
//! Gets the type of representation being used to store data.  If it has no
//! image data, the return value will be GTK2.IMAGE_EMPTY.
//! One of @[IMAGE_ANIMATION], @[IMAGE_EMPTY], @[IMAGE_ICON_NAME], @[IMAGE_ICON_SET], @[IMAGE_IMAGE], @[IMAGE_PIXBUF], @[IMAGE_PIXMAP] and @[IMAGE_STOCK]
//!
//!

GTK2.Image set_from_animation( GTK2.GdkPixbufAnimation anim );
//! Causes the W(Image) to display the given animation.
//!
//!

GTK2.Image set_from_file( string filename );
//! Set the image from a file.
//!
//!

GTK2.Image set_from_icon_name( string icon_name, int size );
//! Sets from an icon name.
//!
//!

GTK2.Image set_from_pixbuf( GTK2.GdkPixbuf pixbuf );
//! Set image from a pixbuf
//!
//!

GTK2.Image set_from_stock( string stock_id, int size );
//! Sets from a stock icon.  Sample icon names are GTK2.STOCK_OPEN,
//! GTK2.STOCK_EXIT.  Sample stock sizes are GTK2.ICON_SIZE_MENU, 
//! GTK2.ICON_SIZE_SMALL_TOOLBAR.  If the stock name isn't known, the image
//! will be empty.
//!
//!

GTK2.Image set_pixel_size( int pixel_size );
//! Sets the pixel size to use for named icons.  If the pixel size is set to
//! a value != -1, it is used instead of the icon size set by
//! set_from_icon_name().
//!
//!
