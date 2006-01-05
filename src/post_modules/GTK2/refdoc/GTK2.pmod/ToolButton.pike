//! Properties that can be notified:
//! icon-widget
//! label
//! label-widget
//! stock-id
//! use-underline
//!
//!
//!  Signals:
//! @b{clicked@}
//!

inherit GTK2.ToolItem;

static GTK2.ToolButton create( GTK2.Widget icon, string|void label );
//! Create a new GTK2.ToolButton.
//! If icon is a string, label should be omitted.  If it isn't,
//! it will be ignored.  The result will be a button from
//! a stock item, one of @[STOCK_ABOUT], @[STOCK_ADD], @[STOCK_APPLY], @[STOCK_BOLD], @[STOCK_CANCEL], @[STOCK_CDROM], @[STOCK_CLEAR], @[STOCK_CLOSE], @[STOCK_COLOR_PICKER], @[STOCK_CONNECT], @[STOCK_CONVERT], @[STOCK_COPY], @[STOCK_CUT], @[STOCK_DELETE], @[STOCK_DIALOG_AUTHENTICATION], @[STOCK_DIALOG_ERROR], @[STOCK_DIALOG_INFO], @[STOCK_DIALOG_QUESTION], @[STOCK_DIALOG_WARNING], @[STOCK_DIRECTORY], @[STOCK_DISCONNECT], @[STOCK_DND], @[STOCK_DND_MULTIPLE], @[STOCK_EDIT], @[STOCK_EXECUTE], @[STOCK_FILE], @[STOCK_FIND], @[STOCK_FIND_AND_REPLACE], @[STOCK_FLOPPY], @[STOCK_FULLSCREEN], @[STOCK_GOTO_BOTTOM], @[STOCK_GOTO_FIRST], @[STOCK_GOTO_LAST], @[STOCK_GOTO_TOP], @[STOCK_GO_BACK], @[STOCK_GO_DOWN], @[STOCK_GO_FORWARD], @[STOCK_GO_UP], @[STOCK_HARDDISK], @[STOCK_HELP], @[STOCK_HOME], @[STOCK_INDENT], @[STOCK_INDEX], @[STOCK_INFO], @[STOCK_ITALIC], @[STOCK_JUMP_TO], @[STOCK_JUSTIFY_CENTER], @[STOCK_JUSTIFY_FILL], @[STOCK_JUSTIFY_LEFT], @[STOCK_JUSTIFY_RIGHT], @[STOCK_LEAVE_FULLSCREEN], @[STOCK_MEDIA_FORWARD], @[STOCK_MEDIA_NEXT], @[STOCK_MEDIA_PAUSE], @[STOCK_MEDIA_PLAY], @[STOCK_MEDIA_PREVIOUS], @[STOCK_MEDIA_RECORD], @[STOCK_MEDIA_REWIND], @[STOCK_MEDIA_STOP], @[STOCK_MISSING_IMAGE], @[STOCK_NETWORK], @[STOCK_NEW], @[STOCK_NO], @[STOCK_OK], @[STOCK_OPEN], @[STOCK_PASTE], @[STOCK_PREFERENCES], @[STOCK_PRINT], @[STOCK_PRINT_PREVIEW], @[STOCK_PROPERTIES], @[STOCK_QUIT], @[STOCK_REDO], @[STOCK_REFRESH], @[STOCK_REMOVE], @[STOCK_REVERT_TO_SAVED], @[STOCK_SAVE], @[STOCK_SAVE_AS], @[STOCK_SELECT_COLOR], @[STOCK_SELECT_FONT], @[STOCK_SORT_ASCENDING], @[STOCK_SORT_DESCENDING], @[STOCK_SPELL_CHECK], @[STOCK_STOP], @[STOCK_STRIKETHROUGH], @[STOCK_UNDELETE], @[STOCK_UNDERLINE], @[STOCK_UNDO], @[STOCK_UNINDENT], @[STOCK_YES], @[STOCK_ZOOM_100], @[STOCK_ZOOM_FIT], @[STOCK_ZOOM_IN] and @[STOCK_ZOOM_OUT].
//! If icon is a GTK2.Widget, it will be used as the icon, and label
//! will be the label.  The label must exists if that is the case.
//!
//!

string get_icon_name( );
//! Returns the name of the themed icon for the tool button.
//!
//!

GTK2.Widget get_icon_widget( );
//! Returns the widget used as the icon widget.
//!
//!

string get_label( );
//! Returns the label used by the tool button, or empty if
//! the tool button doesn't have a label.
//!
//!

GTK2.Widget get_label_widget( );
//! Returns the widget used as the label.
//!
//!

string get_stock_id( );
//! Returns the name of the stock item.
//!
//!

int get_use_underline( );
//! Returns whether underscores in the label property are used
//! as mnemonics on menu items on the overflow menu.
//!
//!

GTK2.ToolButton set_icon_name( string name );
//! Sets the icon for the tool button from a named themed icon.  The
//! "icon-name" property only has an effect if not overridden by non-NULL
//! "label", "icon-widget", and "stock-id" properties.
//!
//!

GTK2.ToolButton set_icon_widget( GTK2.Widget icon_widget );
//! Sets icon_widget as the widget used as the icon on the button.
//!
//!

GTK2.ToolButton set_label( string label );
//! Sets label as the label used for the tool button.
//!
//!

GTK2.ToolButton set_label_widget( GTK2.Widget label_widget );
//! Sets label_widget as the widget used as the label.
//!
//!

GTK2.ToolButton set_stock_id( string stock_id );
//! Sets the name of the stock item.
//!
//!

GTK2.ToolButton set_use_underline( int use_underline );
//! If set, an underline in the label property indicates that
//! the next character should be used for the mnemonic
//! accelerator key in the overflow menu.
//!
//!
