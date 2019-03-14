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

inherit GTK1.Bin;

GTK1.Item deselect( );
//! Emulate a 'deselect' event.
//!
//!

GTK1.Item select( );
//! Emulate a 'select' event.
//!
//!

GTK1.Item toggle( );
//! Emulate a 'toggle' event.
//!
//!
