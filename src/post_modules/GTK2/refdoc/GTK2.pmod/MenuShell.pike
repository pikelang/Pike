//! A GTK2.MenuShell is the abstract base class used to derive the
//! W(Menu) and W(MenuBar) subclasses.
//! 
//! A GTK2.MenuShell is a container of W(MenuItem) objects arranged in a
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
//! shell.  Causes the selection-done signal to be emitted.
//!
//!
//! @b{cycle_focus@}
//!
//! @b{deactivate@}
//! This signal is emitted when a menu shell is deactivated.
//!
//!
//! @b{move_current@}
//! An action signal which moves the current menu item in the direction
//! specified.
//!
//!
//! @b{selection_done@}
//! This signal is emitted when a selection has been completed within a
//! menu shell.
//!
//!

inherit GTK2.Container;

GTK2.MenuShell activate_item( GTK2.Widget menu_item, int force_deactivate );
//! Activates the menu item within the menu shell.
//!
//!

GTK2.MenuShell append( GTK2.Widget what );
//! Adds a new W(MenuItem) to the end of the menu shell's item
//! list. Same as 'add'.
//!
//!

GTK2.MenuShell deactivate( );
//! Deactivates the menu shell. Typically this results in the menu
//! shell being erased from the screen.
//!
//!

GTK2.MenuShell deselect( );
//! Deselects the currently selected item from the menu shell, if any.
//!
//!

array get_children( );
//! This function returns all children of the menushell as an array.
//!
//!

GTK2.MenuShell insert( GTK2.Widget what, int where );
//! Add a widget after the specified location
//!
//!

GTK2.MenuShell prepend( GTK2.Widget what );
//! Add a menu item to the start of the widget (for a menu: top, for a
//! bar: left)
//!
//!

GTK2.MenuShell select_item( GTK2.Widget menuitem );
//! Selects the menu item from the menu shell.
//!
//!
