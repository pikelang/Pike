//! A GTK1.MenuShell is the abstract base class used to derive the
//! W(Menu) and W(MenuBar) subclasses.
//! 
//! A GTK1.MenuShell is a container of W(MenuItem) objects arranged in a
//! list which can be navigated, selected, and activated by the user to
//! perform application functions. A W(MenuItem) can have a submenu
//! associated with it, allowing for nested hierarchical menus.
//! 
//!
//!
//!  Signals:
//! @b{activate_current@}
//! An action signal that activates the current menu item within the
//! menu shell.
//!
//!
//! @b{cancel@}
//! An action signal which cancels the selection within the menu
//! shell. Causes the selection_done signal to be emitted.
//!
//!
//! @b{deactivate@}
//! This signal is emitted when a menu shell is deactivated.
//!
//!
//! @b{move_current@}
//! An action signal which moves the current menu item in the direction
//! specified by the third argument.
//!
//!
//! @b{selection_done@}
//! This signal is emitted when a selection has been completed within a
//! menu shell.
//!
//!

inherit GTK1.Container;

GTK1.MenuShell activate_item( GTK1.Widget menu_item, int force_deactivate );
//! Activates the menu item within the menu shell.
//!
//!

GTK1.MenuShell append( GTK1.Widget what );
//! Adds a new W(MenuItem) to the end of the menu shell's item
//! list. Same as 'add'.
//!
//!

array children( );
//! This function returns all children of the menushell as an array.
//!
//!

GTK1.MenuShell deactivate( );
//! Deactivates the menu shell. Typically this results in the menu
//! shell being erased from the screen.
//!
//!

int get_active( );
//! 1 if the menu shell is currently active.
//!
//!

GTK1.MenuShell insert( GTK1.Widget what, int where );
//! Add a widget after the specified location
//!
//!

GTK1.MenuShell prepend( GTK1.Widget what );
//! Add a menu item to the start of the widget (for a menu: top, for a
//! bar: left)
//!
//!

GTK1.MenuShell select_item( GTK1.Widget menuitem );
//! Selects the menu item from the menu shell.
//!
//!
