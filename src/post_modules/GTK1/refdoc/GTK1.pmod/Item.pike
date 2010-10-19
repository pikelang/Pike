//! This class is inherited by all 'item' type of widgets.
//!
//!
//!  Signals:
//! @b{deselect@}
//!
//! @b{select@}
//!
//! @b{toggle@}
//!

inherit GTK.Bin;

GTK.Item deselect( );
//! Emulate a 'deselect' event.
//!
//!

GTK.Item select( );
//! Emulate a 'select' event.
//!
//!

GTK.Item toggle( );
//! Emulate a 'toggle' event.
//!
//!
