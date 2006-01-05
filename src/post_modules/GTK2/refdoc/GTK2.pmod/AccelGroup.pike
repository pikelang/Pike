//! An AccelGroup stores keybindings.
//! A group is automatically created by W(MenuFactory)
//!
//! NOIMG
//!
//!
//!  Signals:
//! @b{accel_activate@}
//!
//! @b{accel_changed@}
//!

inherit G.Object;

GTK2.AccelGroup connect( int accel_key, int accel_mods, int accel_flags, function cb, mixed user_data );
//! Installs an accelerator in this group.
//!
//!

GTK2.AccelGroup connect_by_path( string accel_path, function cb, mixed user_data );
//! Installs an accelerator in this group, using an accelerator path to look
//! up the appropriate key and modifiers.
//!
//!

static GTK2.AccelGroup create( );
//! Create a new accelerator group
//!
//!

GTK2.AccelGroup disconnect( int accel_key, int accel_mods );
//! Removes an accelerator previously installed.
//!
//!

GTK2.AccelGroup lock( );
//! Locks the group.
//!
//!

GTK2.AccelGroup unlock( );
//! Undoes the last call to lock().
//!
//!
