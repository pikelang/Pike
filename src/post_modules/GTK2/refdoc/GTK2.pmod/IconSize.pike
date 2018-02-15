//! Iconsize.
//!
//!

int from_name( string name );
//! Looks up the icon size associated with name.
//!
//!

string get_name( int size );
//! Gets the canonical name of the given icon size.
//!
//!

int register( string name, int width, int height );
//! Registers a new icon size, along the same lines as GTK2.ICON_SIZE_MENU, etc.
//! Returns the integer value for the size.
//!
//!

GTK2.IconSize register_alias( string alias, int target );
//! Registers alias as another name for target. So calling 
//! GTK2.IconSize->from_name() with alias will return target.
//!
//!
