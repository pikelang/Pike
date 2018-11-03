//! Properties:
//! GTK2.FileChooserDialog dialog
//! int focus-on-click
//! string title
//! int width-chars
//!
//!

inherit GTK2.Hbox;

inherit GTK2.FileChooser;

protected GTK2.FileChooserButton create( string|mapping title_or_props, int|void action );
//! Create a new file-selecting button widget.
//! action is one of @[FILE_CHOOSER_ACTION_CREATE_FOLDER], @[FILE_CHOOSER_ACTION_OPEN], @[FILE_CHOOSER_ACTION_SAVE] and @[FILE_CHOOSER_ACTION_SELECT_FOLDER]
//!
//!

int get_focus_on_click( );
//! Returns whether the button grabs focus when it is clicked.
//!
//!

string get_title( );
//! Retrieves the title of the browse dialog.
//!
//!

int get_width_chars( );
//! Retrieves the width in characters of the button widget's
//! entry and/or label.
//!
//!

GTK2.FileChooserButton set_focus_on_click( int focus );
//! Sets whether the button will grab focus when it is clicked with the mouse.
//! Making mouse clicks not grab focus is useful in places like toolbars where
//! you don't want the keyboard focus removed from the main area of the
//! application.
//!
//!

GTK2.FileChooserButton set_title( string title );
//! Modifies the title of the browse dialog.
//!
//!

GTK2.FileChooserButton set_width_chars( int n_chars );
//! Sets the width (in characters) that the button will use.
//!
//!
