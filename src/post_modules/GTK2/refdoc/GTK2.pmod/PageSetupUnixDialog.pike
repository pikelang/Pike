// Automatically generated from "gtkpagesetupunixdialog.pre".
// Do NOT edit.

//!
//!
//!

inherit GTK2.Dialog;
//!

protected void create( void title_or_props, void parent );
//! Create a new page setup dialog.
//!
//!

GTK2.PageSetup get_page_setup( );
//! Gets the currently selected page setup from the dialog.
//!
//!

GTK2.PrintSettings get_print_settings( );
//! Gets the current print settings from the dialog.
//!
//!

GTK2.PageSetupUnixDialog set_page_setup( GTK2.PageSetup page_setup );
//! Sets the GTK2.PageSetup from which the page setup dialog takes its values.
//!
//!

GTK2.PageSetupUnixDialog set_print_settings( GTK2.PrintSettings print_settings );
//! Sets the GTK2.PrintSettings from which the page setup dialog takes its
//! values.
//!
//!
