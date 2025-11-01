// Automatically generated from "gtkfilechooserwidget.pre".
// Do NOT edit.

//! File chooser widget that can be embedded in other widgets.
//!@expr{ GTK2.FileChooserWidget(GTK2.FILE_CHOOSER_ACTION_OPEN);@}
//!@xml{<image>../images/gtk2_filechooserwidget.png</image>@}
//!
//!
//!

inherit GTK2.Vbox;
//!

inherit GTK2.FileChooser;
//!

protected void create( void action_or_props );
//! Create a new file chooser widget.
//! action is one of @[FILE_CHOOSER_ACTION_CREATE_FOLDER], @[FILE_CHOOSER_ACTION_OPEN], @[FILE_CHOOSER_ACTION_SAVE] and @[FILE_CHOOSER_ACTION_SELECT_FOLDER]
//!
//!
