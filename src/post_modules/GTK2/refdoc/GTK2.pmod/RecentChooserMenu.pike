//!
//!
//!

inherit GTK2.Menu;

inherit GTK2.RecentChooser;

inherit GTK2.Activatable;

protected GTK2.RecentChooserMenu create( mapping|RecentManager props );
//! Create a new GTK2.RecentChooserMenu.
//!
//!

int get_show_numbers( );
//! Returns true if numbers should be shown.
//!
//!

GTK2.RecentChooserMenu set_show_numbers( int show_numbers );
//! Sets whether a number should be added to the items of menu. The numbers
//! are shown to provide a unique character for a mnemonic to be used inside
//! the menu item's label. Only the first items get a number to avoid
//! clashes.
//!
//!
