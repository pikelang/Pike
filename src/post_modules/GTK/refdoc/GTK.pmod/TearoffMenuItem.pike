//! a GTK.TearoffMenuItem is a special W(MenuItem) which is used to
//! tear off and reattach its menu.
//! 
//! When its menu is shown normally, the GTK.TearoffMenuItem is drawn
//! as a dotted line indicating that the menu can be torn
//! off. Activating it causes its menu to be torn off and displayed in
//! its own window as a tearoff menu.
//! 
//! When its menu is shown as a tearoff menu, the GTK.TearoffMenuItem
//! is drawn as a dotted line which has a left pointing arrow graphic
//! indicating that the tearoff menu can be reattached. Activating it
//! will erase the tearoff menu window.
//!@expr{ GTK.TearoffMenuItem()->set_usize( 100,0 )@}
//!@xml{<image>../images/gtk_tearoffmenuitem.png</image>@}
//!
//! 
//!
//!

inherit GTK.MenuItem;

static GTK.TearoffMenuItem create( );
//! Create a new tear of menu item
//!
//!

int get_torn_off( );
//! Return 1 if the menu the menu item is connected to is currently
//! torn off.
//!
//!
