//! An AccelGroup stores keybindings.
//! A group is automatically created by W(MenuFactory)
//!
//! NOIMG
//!
//!

inherit Data;

AccelGroup add_accel( GTK.Widget widget, string signal, int key, int modifiers, int flags );
//! the widget is the one in which the signal specified by 'signal'
//! recides.
//! 
//! The key is the character code (such as 'a' for the a key and '@'
//! for the @ key), and modifiers is a bitmap of one or more bits, the
//! bits are . Flags is one or more of @[ACCEL_VISIBLE], @[ACCEL_SIGNAL_VISIBLE] and @[ACCEL_LOCKED]
//!
//!

static AccelGroup create( );
//! Create a new accelerator group
//!
//!

AccelGroup destroy( );
//!

AccelGroup remove( GTK.Widget widget, int key, int modifiers );
//! Remove all bindings for the specified key/mask in the specified object.
//!
//!
