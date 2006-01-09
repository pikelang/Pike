//! GtkFileChooserDialog should be used to retrieve file or directory names
//! from the user. It will create a new dialog window containing a
//! directory list, and a file list corresponding to the current
//! working directory. The filesystem can be navigated using the
//! directory list, the drop-down history menu, or the TAB key can be
//! used to navigate using filename completion common in text based
//! editors such as emacs and jed.
//! 
//! The default filename can be set using set_filename() and the
//! selected filename retrieved using get_filename().
//! 
//! Use complete() to display files that match a given pattern. This
//! can be used for example, to show only *.txt files, or only files
//! beginning with gtk*.
//! 
//! Simple file operations; create directory, delete file, and rename
//! file, are available from buttons at the top of the dialog. These
//! can be hidden using hide_fileop_buttons() and shown again using
//! show_fileop_buttons().
//! 
//!@expr{ GTK2.FileChooserDialog("File selector", 0, GTK2.FILE_CHOOSER_ACTION_OPEN, ({(["text":"Transmogrify", "id":17]), (["text":"Never mind", "id":42])}))@}
//!@xml{<image>../images/gtk2_filechooserdialog.png</image>@}
//!
//! 
//!
//!

inherit GTK2.Dialog;

inherit GTK2.FileChooser;

static GTK2.FileChooserDialog create( string title, GTK2.Window parent, int mode, array buttons );
//! Creates a new file selection dialog box. By default it will list
//! the files in the current working directory. Operation buttons
//! allowing the user to create a directory, delete files, and rename
//! files will also be present by default.
//!
//!
