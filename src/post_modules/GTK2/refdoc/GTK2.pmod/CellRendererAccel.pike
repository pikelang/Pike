//! Properties:
//! int accel-key
//! int accel-mode
//! int accel-mods
//! int keycode
//!
//!
//!  Signals:
//! @b{accel_cleared@}
//!
//! @b{accel_edited@}
//!

inherit GTK2.CellRendererText;

protected GTK2.CellRendererAccel create( mapping|void props );
//! Create a new GTK2.CellRendererAccel.
//!
//!
