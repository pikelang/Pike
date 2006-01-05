//! Properties:
//! int action
//! GTK2.Widget extra-widget
//! string file-system-backend
//! GTK2.FileFilter filter
//! int local-only
//! GTk.Widget preview-widget
//! int preview-widget-active
//! int select-multiple
//! int show-hidden
//! int use-preview-label
//!
//!
//!  Signals:
//! @b{current_folder_changed@}
//!
//! @b{file_activated@}
//!
//! @b{selection_changed@}
//!
//! @b{update_preview@}
//!

int add_shortcut_folder( string folder );
//! Adds a folder to be displayed with the shortcut folders in a file chooser.
//! Note that shortcut folders do not get saved, as they are provided by the
//! application.  For example, you can use this to add a 
//! "/usr/share/mydrawprogram/Clipart" folder to the volume list.
//!
//!

int add_shortcut_folder_uri( string uri );
//! Adds a folder URI to be displayed with the shortcut folders.
//!
//!

int get_action( );
//! Get the type of action the file chooser is performing.
//!
//!

string get_current_folder( );
//! Gets the current folder as a local filename.
//!
//!

string get_current_folder_uri( );
//! Gets the current folder as a URI.
//!
//!

int get_do_overwrite_confirmation( );
//! Queries whether a file chooser is set to confirm for overwriting when the
//! user types a file name that already exists.
//!
//!

GTK2.Widget get_extra_widget( );
//! Gets the extra widget.
//!
//!

string get_filename( );
//! Gets the filename for the currently select file.
//!
//!

array get_filenames( );
//! Get a list of all selected file and subfolders.
//! The returned names are full absolute paths.
//! If files in the current folder cannot be represented as local
//! filenames, they will be ignored.  If you want those files,
//! You may want to use get_uris() instead.
//!
//!

int get_local_only( );
//! Gets whether only local files can be selected in the file selector.
//!
//!

string get_preview_filename( );
//! Gets the filename that should be previewed in a custom preview widget.
//!
//!

string get_preview_uri( );
//! Gets the URI that should be previewed in a custom preview widget.
//!
//!

GTK2.Widget get_preview_widget( );
//! Gets the current preview widget.
//!
//!

int get_preview_widget_active( );
//! Gets whether the preview widget set by set_preview_widget_active() should
//! be shown for the current filename.
//!
//!

int get_select_multiple( );
//! Gets whether multiple files can be selected.
//!
//!

int get_show_hidden( );
//! Gets whether hidden files and folders are displayed.
//!
//!

string get_uri( );
//! Gets the URI for the currently selected file.
//!
//!

array get_uris( );
//! List all the selected files and subfolders in the current folder.
//! The returned names are full absolute URIs.
//!
//!

int get_use_preview_label( );
//! Gets whether a stock label should be drawn with the name of the previewed
//! file.
//!
//!

array list_shortcut_folder_uris( );
//! Queries the list of shortcut folders.
//!
//!

array list_shortcut_folders( );
//! Queries the list of shortcut folders.
//!
//!

GTK2.FileChooser remove_shortcut_folder( string folder );
//! Removes a folder from the list of shortcut folders.
//!
//!

GTK2.FileChooser remove_shortcut_folder_uri( string uri );
//! Removes a folder URI.
//!
//!

GTK2.FileChooser select_all( );
//! Select all files.
//!
//!

int select_filename( string filename );
//! Selects a filename.
//!
//!

int select_uri( string uri );
//! Selects the file by uri.
//!
//!

GTK2.FileChooser set_action( int action );
//! Sets type of action, from opening or saving a file, or 
//! opening or saving a folder.
//! Options are:
//!  @[FILE_CHOOSER_ACTION_OPEN]
//!  @[FILE_CHOOSER_ACTION_SAVE]
//!  @[FILE_CHOOSER_ACTION_SELECT_FOLDER]
//!  @[FILE_CHOOSER_ACTION_CREATE_FOLDER]
//!
//!

int set_current_folder( string folder );
//! Sets the current folder from a local filename.  The user
//! will be shown the full contents of the current folder, plus
//! user interface elements for navigating to other folders.
//!
//!

int set_current_folder_uri( string uri );
//! Sets the current folder from a URI.
//!
//!

GTK2.FileChooser set_current_name( string filename );
//! Sets current name.
//!
//!

GTK2.FileChooser set_do_overwrite_confirmation( int setting );
//! Sets whether a file chooser in GTK2.FILE_CHOOSER_ACTION_SAVE mode will
//! present a confirmation dialog if the users types a file name that already
//! exists.  This is FALSE by default.
//! 
//! Regardless of this setting, the chooser will emit the "confirm-overwrite"
//! signal when appropriate.
//! 
//! If all you need is the stock confirmation dialog, set this property to TRUE.
//! You can override the way confirmation is done by actually handling the
//! "confirm-overwrite" signal.
//!
//!

GTK2.FileChooser set_extra_widget( GTK2.Widget widget );
//! Sets an application-supplied widget to provide extra options to the user.
//!
//!

int set_filename( string filename );
//! Sets filename as the current filename.
//! Same as unselect_all() followed by select_filename();
//!
//!

GTK2.FileChooser set_local_only( int local_only );
//! Sets whether on local files can be selected.
//!
//!

GTK2.FileChooser set_preview_widget( GTK2.Widget widget );
//! Sets an application-supplied widget to use to display a custom preview of
//! the currently selected file.  To implement a preview, after setting the
//! preview widget, you connect to the "update-preview" signal, and call
//! get_preview_filename() or get_preview_uri() on each change.  If you can
//! display a preview of the new file, update your widget and set the preview
//! active using set_preview_widget_active().  Otherwise, set the preview
//! inactive.
//! 
//! When there is no application-supplied preview widget, or the application-
//! supplied preview widget is not active, the file chooser may display an
//! internally generated preview of the current file or it may display no
//! preview at all.
//!
//!

GTK2.FileChooser set_preview_widget_active( int setting );
//! Sets whether the preview widget set by set_preview_widget() should be shown
//! for the current filename.  When setting is set to false, the file chooser
//! may display an internally generated preview of the current file or it may
//! display no preview at all.
//!
//!

GTK2.FileChooser set_select_multiple( int select_multiple );
//! Sets whether multiple files can be selected in the file selector.
//! This is only relevant if the action is set to be
//! @[FILE_CHOOSER_ACTION_OPEN] or
//! @[FILE_CHOOSER_ACTION_SAVE]
//!
//!

GTK2.FileChooser set_show_hidden( int setting );
//! Sets whether hidden files and folders are displayed.
//!
//!

int set_uri( string uri );
//! Sets the uri as the current file.
//!
//!

GTK2.FileChooser set_use_preview_label( int setting );
//! Sets whether the file chooser should display a stock label with the name
//! of the file that is being previewed; the default is true.  Applications
//! that want to draw the whole preview area themselves should set this to
//! false and display the name themselves in their preview widget.
//!
//!

GTK2.FileChooser unselect_all( );
//! Unselect all files.
//!
//!

GTK2.FileChooser unselect_filename( string filename );
//! Unselects a currently selected filename.
//!
//!

GTK2.FileChooser unselect_uri( string uri );
//! Unselect the uri.
//!
//!
