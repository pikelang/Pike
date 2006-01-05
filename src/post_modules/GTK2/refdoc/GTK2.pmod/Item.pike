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

inherit GTK2.Bin;

GTK2.Item deselect( );
//! Emulate a 'deselect' event.
//!
//!

GTK2.Item select( );
//! Emulate a 'select' event.
//!
//!

GTK2.Item toggle( );
//! Emulate a 'toggle' event.
//!
//!
