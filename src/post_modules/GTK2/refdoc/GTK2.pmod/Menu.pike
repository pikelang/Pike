//! A GTK2.Menu is a W(MenuShell) that implements a drop down menu
//! consisting of a list of W(MenuItem) objects which can be navigated
//! and activated by the user to perform application functions.
//! 
//! A GTK2.Menu is most commonly dropped down by activating a W(MenuItem)
//! in a W(MenuBar) or popped up by activating a W(MenuItem) in another
//! GTK2.Menu.
//! 
//! A GTK2.Menu can also be popped up by activating a
//! W(OptionMenu). Other composite widgets such as the W(Notebook) can
//! pop up a GTK2.Menu as well.
//! 
//! Applications can display a GTK2.Menu as a popup menu by calling the
//! popup() function. The example below shows how an application can
//! pop up a menu when the 3rd mouse button is pressed.
//! 
//! @pre{
//!   GTK2.Menu menu = create_menu();
//!   GTK2.Window window = create_window();
//!   window->signal_connect( "button_press_event", lambda(GTK2.Menu m,
//!                                                        GTK2.Window w,
//!                                                        mapping e ) {
//!             if( e->button == 3 )
//!              menu->popup();
//!          }, menu );
//! @}
//! Properties:
//! int tearoff-state
//! string tearoff-title
//! 
//! Child properties:
//! int bottom-attach
//! int left-attach
//! int right-attach
//! int top-attach
//! 
//! Style properties:
//! int horizontal-offset
//! int vertical-offset
//! int vertical-padding
//!
//!
//!  Signals:
//! @b{move_scroll@}
//!

inherit GTK2.MenuShell;

GTK2.Menu attach( GTK2.Widget child, int left_attach, int right_attach, int top_attach, int bottom_attach );
//! Adds a new W(MenuItem) to a (table) menu.  The number of 'cells'
//! that an item will occupy is specified by left_attach, right_attach,
//! top_attach, and bottom_attach.  These each represent the
//! leftmost, rightmost, uppermost and lower column row numbers of
//! the table.
//!
//!

static GTK2.Menu create( mapping|void props );
//! Creates a new GTK2.Menu widget.
//!
//!

GTK2.Menu detach( );
//! Detaches the menu from the widget to which it had been attached.
//!
//!

GTK2.AccelGroup get_accel_group( );
//! Gets the W(AccelGroup) which holds global accelerators for the menu.
//!
//!

GTK2.Widget get_active( );
//! Returns the selected menu item from the menu.  This is used by
//! the W(OptionMenu).
//!
//!

GTK2.Widget get_attach_widget( );
//! Returns the W(Widget) that the menu is attached to.
//!
//!

array get_for_attach_widget( );
//! Returns a list of the menus which are attached to this widget.
//!
//!

int get_tearoff_state( );
//! Returns whether the menu is torn off.
//!
//!

string get_title( );
//! Returns the title of the menu.
//!
//!

GTK2.Menu popdown( );
//! Removes the menu from the screen.
//!
//!

GTK2.Menu popup( int|void button_pressed_to_show_menu );
//! The default button is 3.
//!
//!

GTK2.Menu reorder_child( GTK2.Widget child, int position );
//! Moves a W(MenuItem) to a new position within the GTK2.Menu.
//!
//!

GTK2.Menu reposition( );
//! Repositions the menu according to its position function.
//!
//!

GTK2.Menu set_accel_group( GTK2.AccelGroup accelerators );
//! Set the W(AccelGroup) which holds global accelerators for the menu.
//!
//!

GTK2.Menu set_accel_path( string accel_path );
//! Sets an accelerator path for this menu.
//!
//!

GTK2.Menu set_active( int activep );
//! Selects the specified menu item within the menu. This is used by
//! the W(OptionMenu).
//!
//!

GTK2.Menu set_tearoff_state( int torn_off );
//! Changes the tearoff state of the menu.  A menu is normally displayed
//! as a drop down menu which persists as long as the menu is active.  It
//! can also be displayed as a tearoff menu which persists until it is
//! closed or reattached.
//!
//!

GTK2.Menu set_title( string new_title );
//! Sets the title string for the menu.  The title is displayed when the
//! menu is shown as a tearoff menu.
//!
//!
