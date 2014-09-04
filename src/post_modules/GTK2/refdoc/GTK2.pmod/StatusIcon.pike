//! The "system tray" or notification area is normally used for transient
//! icons that indicate some special state. For example, a system tray icon
//! might appear to tell the user that they have new mail, or have an
//! incoming instant message, or something along those lines. The basic
//! idea is that creating an icon in the notification area is less annoying
//! than popping up a dialog.
//! 
//! A GTK2.StatusIcon object can be used to display an icon in a "system tray".
//! The icon can have a tooltip, and the user can interact with it by
//! activating it or popping up a context menu. Critical information should/
//! not solely be displayed in a GTK2.StatusIcon, since it may not be visible
//! (e.g. when the user doesn't have a notification area on his panel). This
//! can be checked with is_embedded().
//! 
//! On X11, the implementation follows the freedesktop.org "System Tray"
//! specification. Implementations of the "tray" side of this specification
//! can be found e.g. in the GNOME and KDE panel applications.
//! 
//! Note that a GTK2.StatusIcon is not a widget, but just a G.Object. Making
//! it a widget would be impractical, since the system tray on Win32 doesn't
//! allow to embed arbitrary widgets.
//! Properties:
//! int blinking
//! string file
//! string icon-name
//! GDK2.Pixbuf pixbuf
//! int size
//! string stock
//! int storage-type
//! int visible
//!
//!
//!  Signals:
//! @b{activate@}
//!
//! @b{popup_menu@}
//!
//! @b{size_changed@}
//!

inherit G.Object;

protected GTK2.StatusIcon create( mapping|GdkPixbuf|string arg );
//! Create a new GTK2.StatusIcon.
//!
//!

int get_blinking( );
//! Returns whether the icon is blinking.
//!
//!

string get_icon_name( );
//! Gets the name of the icon.
//!
//!

GTK2.GdkPixbuf get_pixbuf( );
//! Get the pixbuf.
//!
//!

int get_size( );
//! Gets the size in pixels.
//!
//!

string get_stock( );
//! Gets the id of the stock icon.
//!
//!

int get_storage_type( );
//! Gets the type of icon, e.g. GTK2.IMAGE_PIXMAP, GTK2.IMAGE_PIXBUF, etc.
//!
//!

int get_visible( );
//! Returns whether the icon is visible or not.
//!
//!

int is_embedded( );
//! Returns whether the status icon is embedded in a notification area.
//!
//!

GTK2.StatusIcon set_blinking( int blinking );
//! Makes the status icon start or stop blinking.
//!
//!

GTK2.StatusIcon set_from_file( string filename );
//! Set the icon from a file.
//!
//!

GTK2.StatusIcon set_from_icon_name( string icon_name );
//! Set the icon from the icon called icon_name from the current theme.
//!
//!

GTK2.StatusIcon set_from_pixbuf( GTK2.GdkPixbuf pixbuf );
//! Set the icon from pixbuf.
//!
//!

GTK2.StatusIcon set_from_stock( string stock_id );
//! Set the icon from a stock icon.
//!
//!

GTK2.StatusIcon set_tooltip( string tooltip_text );
//! Sets the tooltip.
//!
//!

GTK2.StatusIcon set_visible( int visible );
//! Shows or hides a status icon.
//!
//!
