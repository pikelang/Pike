// Automatically generated from "gtkprintjob.pre".
// Do NOT edit.

//! A GTK2.PrintJob object represents a job that is sent to a printer.
//! You only need to deal directly with print jobs if you use the non-portable
//! GTK2.PrintUnixDialog API.
//! Properties:
//! GTK2.PageSetup page-setup
//! GTK2.Printer printer
//! GTK2.PrintSettings settings
//! string title
//! int track-print-status
//!
//!
//!  Signals:
//! @b{status_changed@}
//!

inherit G.Object;
//!

protected void create( void props_or_title, void printer, void settings, void page_setup );
//! Create a new GTK2.PrintJob.
//!
//!

GTK2.Printer get_printer( );
//! Gets the printer of the print job.
//!
//!

GTK2.PrintSettings get_settings( );
//! Gets the settings of the print job.
//!
//!

int get_status( );
//! Gets the status of the print job.
//!
//!

string get_title( );
//! Gets the job's title.
//!
//!

int get_track_print_status( );
//! Returns whether jobs will be tracked after printing.
//!
//!

int set_source_file( string filename );
//! Make the GTK2.PrintJob send an existing document to the printing system.
//! The file can be in any format understood by the platforms printing system
//! (typically PostScript, but on many platforms PDF may work too).
//!
//!

GTK2.PrintJob set_track_print_status( int track_status );
//! If track_status is TRUE, the print job will try to continue report on the
//! status of the print job in the printer queues and printer. This can allow
//! your application to show things like "out of paper" issues, and when the
//! print job actually reaches the printer.
//! 
//! This function is often implemented using some form of polling, so it should
//! not be enabled unless needed.
//!
//!
