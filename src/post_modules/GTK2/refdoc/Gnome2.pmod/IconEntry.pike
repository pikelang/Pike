// Automatically generated from "gnomeiconentry.pre".
// Do NOT edit.

//! This widget provides the facilities to select an icon. An icon is
//! displayed inside a button, when the button is pressed, an Icon
//! selector (a dialog with a W(GnomeIconSelection) widget) pops up to let
//! the user choose an icon. It also allows one to Drag and Drop the
//! images to and from the preview button.
//! Properties:
//! string browse-dialog-title
//! string filename
//! string history-id
//! GTK2.Dialog pick-dialog
//! string pixmap-subdir
//!
//!
//!  Signals:
//! @b{browse@}
//!
//! @b{changed@}
//!

inherit GTK2.Vbox;
//!

protected void create( void history_id, void title );
//! Creates a new icon entry widget
//!
//!

string get_filename( );
//! Gets the file name of the image if it was possible to load it into the
//! preview.  That is, it will only return a filename if the image exists and
//! it was possible to load it as an image.
//!
//!

GTK2.Widget pick_dialog( );
//! If a pick dialog exists, returns it.  This is if you need to do something
//! with all dialogs.  You would use the browse signal with connect_after to
//! get the pick dialog when it is displayed.
//!
//!

Gnome2.IconEntry set_browse_dialog_title( string title );
//! Set the title of the browse dialog.
//!
//!

int set_filename( string filename );
//! Sets the icon of Gnome2.IconEntry to be the one pointed to by filename.
//!
//!

Gnome2.IconEntry set_history_id( string history_id );
//! Set the history_id of the entry in the browse dialog and reload the
//! history.
//!
//!

Gnome2.IconEntry set_pixmap_subdir( string subdir );
//! Sets the subdirectory below gnome's default pixmap directory to use
//! as the default path for the file entry.
//!
//!
