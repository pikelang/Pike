//! The GNOME icon list widget can hold a number of icons with
//! captions. The icons on the list can be selected (various selection
//! methods are supported). The programmer can enable caption-editing
//! for the icons. This parameters is configured when you create the
//! icon list widget.
//!  You can control the type of selection mode you desire by using
//! the set_selection_mode() function.
//!
//!
//!  Signals:
//! @b{select_icon@}
//!
//! @b{text_changed@}
//!
//! @b{unselect_icon@}
//!

inherit GTK.Widget;

Gnome.IconList append( string icon_filename, string text );
//! Appends an icon to the specified icon list. The icon's image is
//! loaded from the specified file, and it is inserted at the pos
//! index.
//!
//!

Gnome.IconList clear( );
//! Clears the contents for the icon list by removing all the icons. If
//! destroy handlers were specified for any of the icons, they will be
//! called with the appropriate data.
//!
//!

static Gnome.IconList create( int icon_widt, int flags );
//! Creates a new icon list widget. The icon columns are allocated a
//! width of icon_width pixels. Icon captions will be word-wrapped to
//! this width as well.
//! 
//! If flags has the Gnome.IconListIsEditable flag set, then the
//! user will be able to edit the text in the icon captions, and the
//! "text_changed" signal will be emitted when an icon's text is
//! changed.
//!
//!

int find_icon_from_data( object data );
//! Find a icon in the list that has the given user data.  If no icon is
//! found, -1 is returned.
//!
//!

Gnome.IconList freeze( );
//! Freezes an icon list so that any changes made to it will not be
//! reflected on the screen until it is thawed with thaw(). It is
//! recommended to freeze the icon list before inserting or deleting
//! many icons, for example, so that the layout process will only be
//! executed once, when the icon list is finally thawed.
//! 
//! You can call this function multiple times, but it must be balanced
//! with the same number of calls to thaw() before the changes will
//! take effect.
//!
//!

int get_icon_at( int x, int y );
//! Returns the index of the icon that is under the specified
//! coordinates, which are relative to the icon list's window. If there
//! is no icon in that position, -1 is returned.
//!
//!

object get_icon_data( int icon );
//! Return the data associated with a icon, or 0.
//!
//!

array get_selected_icons( );
//! Return an array with the currently selected icons
//!
//!

int icon_is_visible( int pos );
//! returns 1 if the icon whose index is pos is visible.
//!
//!

Gnome.IconList insert( int pos, string icon_filename, string text );
//!  Inserts an icon in the specified icon list. The icon's image is
//!  loaded from the specified file, and it is inserted at the pos
//!  index.
//!
//!

Gnome.IconList moveto( int pos, float yalign );
//! Makes the icon whose index is pos be visible on the screen. The
//! icon list gets scrolled so that the icon is visible. An alignment
//! of 0.0 represents the top of the visible part of the icon list, and
//! 1.0 represents the bottom. An icon can be centered on the icon
//! list using 0.5 as the yalign.
//!
//!

Gnome.IconList remove( int pos );
//! Removes the icon at index position pos. If a destroy handler was
//! specified for that icon, it will be called with the appropriate
//! data.
//!
//!

Gnome.IconList select_icon( int idx );
//! Selects the specified icon.
//!
//!

Gnome.IconList set_col_spacing( int pixels );
//! Sets the spacing to be used between columns of icons.
//!
//!

Gnome.IconList set_hadjustment( GTK.Adjustment hadj );
//!  Sets the adjustment to be used for horizontal scrolling.  This is
//!  normally not required, as the icon list can be simply inserted in
//!  a W(ScrolledWindow) and scrolling will be handled automatically.
//!
//!

Gnome.IconList set_icon_border( int pixels );
//! Set the width of the border to be displayed around an icon's image.
//! This is currently not implemented.
//!
//!

Gnome.IconList set_icon_data( int icon, object data );
//! Set the user data associated with the specified icon.
//! This data can be used to find icons, and when an icon is selected it
//! can be easily retrieved using get_icon_data.
//! 
//! @b{You can only use objects as icon data right now@}
//!
//!

Gnome.IconList set_icon_width( int w );
//! Sets the amount of horizontal space allocated to the icons,
//! i.e. the column width of the icon list
//!
//!

Gnome.IconList set_row_spacing( int pixels );
//! Sets the spacing to be used between rows of icons.
//!
//!

Gnome.IconList set_selection_mode( int mode );
//! One of @[SELECTION_BROWSE], @[SELECTION_EXTENDED], @[SELECTION_MULTIPLE] and @[SELECTION_SINGLE].
//!
//!

Gnome.IconList set_separators( string sep );
//! Set the characters that can be used as word separators when doing
//! word-wrapping of the text captions.
//!
//!

Gnome.IconList set_text_spacing( int pixels );
//! Sets the spacing to be used between the icon and its caption
//!
//!

Gnome.IconList set_vadjustment( GTK.Adjustment hadj );
//!  Sets the adjustment to be used for vertical scrolling.  This is
//!  normally not required, as the icon list can be simply inserted in
//!  a W(ScrolledWindow) and scrolling will be handled automatically.
//!
//!

Gnome.IconList thaw( );
//! Unfreeze the icon list
//!
//!

Gnome.IconList unselect_all( );
//! Unselect all icons.
//!
//!

Gnome.IconList unselect_icon( int idx );
//! Unselects the specified icon.
//!
//!
