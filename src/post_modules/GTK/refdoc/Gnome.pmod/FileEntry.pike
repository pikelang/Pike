//! This widget provides an entry box with history (a W(GnomeEntry))
//! and a button which can pop up a file selector dialog box
//! W(GtkFileSelection). It also accepts DND drops from the filemanager
//! and other sources.
//!@code{ Gnome.FileEntry("","")@}
//!@xml{<image>../images/gnome_fileentry.png</image>@}
//!
//!
//!
//!  Signals:
//! @b{browse_clicked@}
//! Signal emitted when the "browse" button is clicked. This is so that
//! you can add stuff to the file selector or to override this method.
//!
//!

inherit GTK.Hbox;

static Gnome.FileEntry create( string history_id, string browse_dialog_title );
//! Creates a new Gnome.FileEntry widget.
//!
//!

string get_full_path( int file_must_exist );
//! Gets the full absolute path of the file from the entry. If
//! file_must_exist is false, nothing is tested and the path is
//! returned. If file_must_exist is true, then the path is only
//! returned if the path actually exists. In case the entry is a
//! directory entry (see set_directory), then if the path exists and is
//! a directory then it's returned; if not, it is assumed it was a file
//! so we try to strip it, and try again.
//!
//!

Gnome.Entry gnome_entry( );
//! Get the W(GnomeEntry) component of the widget for lower-level
//! manipulation.
//!
//!

GTK.Entry gtk_entry( );
//! Get the W(Entry) component of the widget for lower-level
//! manipulation.
//!
//!

Gnome.FileEntry set_default_path( string path );
//! Set the default path of browse dialog to path. The default path is
//! only used if the entry is empty or if the current path of the entry
//! is not an absolute path, in which case the default path is
//! prepended to it before the dialog is started.
//!
//!

Gnome.FileEntry set_directory( int directory_entry );
//! Sets whether this is a directory only entry. If directory_entry is
//! true, then get_full_path will check for the file being a directory,
//! and the browse dialog will have the file list disabled.
//!
//!

Gnome.FileEntry set_modal( int is_modal );
//! Sets the modality of the browse dialog.
//!
//!

Gnome.FileEntry set_title( string browse_dialog_title );
//! Set the title of the browse dialog to browse_dialog_title. The new
//! title will go into effect the next time the browse button is
//! pressed.
//!
//!
