//! Entry for large images with a preview. Unlike GnomeIconEntry, it
//! does not scale the images to a specific size and shows them
//! 1:1. This is perfect for selection of backgrounds and such. It also
//! allows DND to be performed on the preview box. It also provides all
//! the GnomeEntry functionality as well.
//!@code{ Gnome.PixmapEntry("","browse...",1);@}
//!@xml{<image>../images/gnome_pixmapentry.png</image>@}
//!
//!
//!

inherit GTK.Vbox;

static Gnome.PixmapEntry create( string history_id, string browse_dialog_title, int do_preview );
//! Creates a new pixmap entry widget, if do_preview is false, the
//! preview is hidden but the files are still loaded so that it's easy
//! to show it. For a pixmap entry without preview, use the
//! W(GnomeFileEntry) widget.
//!
//!

string get_filename( );
//! Gets the filename of the image if the preview successfully loaded.
//!
//!

Gnome.Entry gnome_entry( );
//! Get the W(GnomeEntry) component of the W(GnomePixmapEntry) widget
//! for lower-level manipulation.
//!
//!

Gnome.FileEntry gnome_file_entry( );
//! Get the W(GnomeFileEntry) component of the W(GnomePixmapEntry)
//! widget for lower-level manipulation.
//!
//!

Gnome.PixmapEntry set_pixmap_subdir( string dir );
//! Sets the default path for the file entry. The new subdirectory
//! should be specified relative to the default GNOME pixmap directory.
//!
//!

Gnome.PixmapEntry set_preview( int do_preview );
//! Sets whether or not previews of the currently selected pixmap
//! should be shown in the file selector.
//!
//!

Gnome.PixmapEntry set_preview_size( int min_w, int min_h );
//! Sets the minimum size of the preview frame in pixels.
//!
//!
