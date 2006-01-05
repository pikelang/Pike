//! Properties to be notified.
//! GTK2.Menu menu
//!
//!
//!  Signals:
//! @b{show_menu@}
//!

inherit GTK2.ToolButton;

static GTK2.MenuToolButton create( GTK2.Widget icon, string|void label );
//! Create a new GTK2.MenuToolButton.
//! If icon is a string, label should be omitted.  If it isn't,
//! it will be igrnored.  The result will be a button from
//! a stock item, one of @[STOCK_ABOUT], @[STOCK_ADD], @[STOCK_APPLY], @[STOCK_BOLD], @[STOCK_CANCEL], @[STOCK_CDROM], @[STOCK_CLEAR], @[STOCK_CLOSE], @[STOCK_COLOR_PICKER], @[STOCK_CONNECT], @[STOCK_CONVERT], @[STOCK_COPY], @[STOCK_CUT], @[STOCK_DELETE], @[STOCK_DIALOG_AUTHENTICATION], @[STOCK_DIALOG_ERROR], @[STOCK_DIALOG_INFO], @[STOCK_DIALOG_QUESTION], @[STOCK_DIALOG_WARNING], @[STOCK_DIRECTORY], @[STOCK_DISCONNECT], @[STOCK_DND], @[STOCK_DND_MULTIPLE], @[STOCK_EDIT], @[STOCK_EXECUTE], @[STOCK_FILE], @[STOCK_FIND], @[STOCK_FIND_AND_REPLACE], @[STOCK_FLOPPY], @[STOCK_FULLSCREEN], @[STOCK_GOTO_BOTTOM], @[STOCK_GOTO_FIRST], @[STOCK_GOTO_LAST], @[STOCK_GOTO_TOP], @[STOCK_GO_BACK], @[STOCK_GO_DOWN], @[STOCK_GO_FORWARD], @[STOCK_GO_UP], @[STOCK_HARDDISK], @[STOCK_HELP], @[STOCK_HOME], @[STOCK_INDENT], @[STOCK_INDEX], @[STOCK_INFO], @[STOCK_ITALIC], @[STOCK_JUMP_TO], @[STOCK_JUSTIFY_CENTER], @[STOCK_JUSTIFY_FILL], @[STOCK_JUSTIFY_LEFT], @[STOCK_JUSTIFY_RIGHT], @[STOCK_LEAVE_FULLSCREEN], @[STOCK_MEDIA_FORWARD], @[STOCK_MEDIA_NEXT], @[STOCK_MEDIA_PAUSE], @[STOCK_MEDIA_PLAY], @[STOCK_MEDIA_PREVIOUS], @[STOCK_MEDIA_RECORD], @[STOCK_MEDIA_REWIND], @[STOCK_MEDIA_STOP], @[STOCK_MISSING_IMAGE], @[STOCK_NETWORK], @[STOCK_NEW], @[STOCK_NO], @[STOCK_OK], @[STOCK_OPEN], @[STOCK_PASTE], @[STOCK_PREFERENCES], @[STOCK_PRINT], @[STOCK_PRINT_PREVIEW], @[STOCK_PROPERTIES], @[STOCK_QUIT], @[STOCK_REDO], @[STOCK_REFRESH], @[STOCK_REMOVE], @[STOCK_REVERT_TO_SAVED], @[STOCK_SAVE], @[STOCK_SAVE_AS], @[STOCK_SELECT_COLOR], @[STOCK_SELECT_FONT], @[STOCK_SORT_ASCENDING], @[STOCK_SORT_DESCENDING], @[STOCK_SPELL_CHECK], @[STOCK_STOP], @[STOCK_STRIKETHROUGH], @[STOCK_UNDELETE], @[STOCK_UNDERLINE], @[STOCK_UNDO], @[STOCK_UNINDENT], @[STOCK_YES], @[STOCK_ZOOM_100], @[STOCK_ZOOM_FIT], @[STOCK_ZOOM_IN] and @[STOCK_ZOOM_OUT].
//! If icon is a GTK2.Widget, it will be used as the icon, and label
//! will be the label.  The label must exist if that is the case.
//!
//!

GTK2.Widget get_menu( );
//! Returns the GTK2.Menu.
//!
//!

GTK2.MenuToolButton set_arrow_tooltip( GTK2.Tooltips tooltips, string tip_text, string tip_private );
//! Sets the GTK2.Tooltips object to be used for the arrow button
//! which pops up the menu.
//!
//!

GTK2.MenuToolButton set_menu( GTK2.Widget menu );
//! Sets the GTK2.Menu that is popped up when the user clicks on the arrow.
//!
//!
