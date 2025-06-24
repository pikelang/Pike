// Automatically generated from "gtkprintunixdialog.pre".
// Do NOT edit.

//! Properties:
//! int current-page
//! GTK2.PageSetup page-setup
//! GTK2.PrintSettings print-settings
//! GTK2.Printer selected-printer
//!
//!

inherit GTK2.Dialog;
//!

GTK2.PrintUnixDialog add_custom_tab( GTK2.Widget child, GTK2.Widget label );
//! Adds a custom tab to the print dialog.
//!
//!

protected void create( void title_or_props, void parent );
//! Creates a new print unix dialog.
//!
//!

int get_current_page( );
//! Gets the current page of this dialog.
//!
//!

GTK2.PageSetup get_page_setup( );
//! Gets the page setup that is used.
//!
//!

GTK2.Printer get_selected_printer( );
//! Gets the currently selected printer.
//!
//!

GTK2.PrintSettings get_settings( );
//! Gets the current print settings from the dialog.
//!
//!

GTK2.PrintUnixDialog set_current_page( int page );
//! Sets the current page number.  If page is not -1, this enables the
//! current page choice for the range of pages to print.
//!
//!

GTK2.PrintUnixDialog set_manual_capabilities( int capabilities );
//! This lets you specify the printing capabilities your application supports.
//! For instance, if you can handle scaling the output then you pass
//! GTK2.PRINT_CAPABILITY_SCALE.  If you don't pass that, then the dialog will
//! only let you select the scale if the printing system automatically
//! handles scaling.
//!
//!

GTK2.PrintUnixDialog set_page_setup( GTK2.PageSetup page_setup );
//! Sets the page setup of this dialog.
//!
//!

GTK2.PrintUnixDialog set_settings( GTK2.PrintSettings print_settings );
//! Sets the GTK2.PrintSettings from which the page setup dialog takes its
//! values.
//!
//!
