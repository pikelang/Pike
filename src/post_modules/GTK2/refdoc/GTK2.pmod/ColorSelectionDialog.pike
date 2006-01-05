//! Color Selection Dialog
//!
//!

inherit GTK2.Dialog;

static GTK2.ColorSelectionDialog create( string|mapping title_or_props );
//! Create a new Color Selection Dialog
//!
//!

GTK2.Widget get_cancel_button( );
//! The Cancel button.
//!
//!

GTK2.Widget get_colorsel( );
//! The Color Selection widget contained within the dialog
//!
//!

GTK2.Widget get_help_button( );
//! The Help button.
//!
//!

GTK2.Widget get_ok_button( );
//! The OK button.
//!
//!
