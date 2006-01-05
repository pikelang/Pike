//! A GTK2.TearoffMenuItem is a special W(MenuItem) which is used to
//! tear off and reattach its menu.
//! 
//! When its menu is shown normally, the GTK2.TearoffMenuItem is drawn
//! as a dotted line indicating that the menu can be torn
//! off. Activating it causes its menu to be torn off and displayed in
//! its own window as a tearoff menu.
//! 
//! When its menu is shown as a tearoff menu, the GTK2.TearoffMenuItem
//! is drawn as a dotted line which has a left pointing arrow graphic
//! indicating that the tearoff menu can be reattached. Activating it
//! will erase the tearoff menu window.
//!@expr{ GTK2.TearoffMenuItem()->set_size_request(100,0)@}
//!@xml{<image>../images/gtk2_tearoffmenuitem.png</image>@}
//!
//! 
//!
//!

inherit GTK2.MenuItem;

static GTK2.TearoffMenuItem create( mapping|void props );
//! Create a new tearoff menu item
//!
//!
