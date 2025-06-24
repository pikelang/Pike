// Automatically generated from "gtkprinter.pre".
// Do NOT edit.

//! A GTK2.Printer object represents a printer. You only need to deal
//! directly with printers if you use the non-portable GTK2.PrintUnixDialog API.
//! Properties:
//! int accepts-pdf
//! int accepts-ps
//! GTK2.PrintBackend backend
//! string icon-name
//! int is-virtual
//! int job-count
//! string location
//! string name
//! string state-message
//!
//!
//!  Signals:
//! @b{details_acquired@}
//!

inherit G.Object;
//!

int accepts_pdf( );
//! Returns whether the printer accepts input in PDF format.
//!
//!

int accepts_ps( );
//! Returns whether the printer accepts input in PostScript format.
//!
//!

int compare( GTK2.Printer b );
//! Compares two printers.
//!
//!

protected void create( void props_or_name, void backend, void virtual );
//! Create a new GTK2.Printer.
//!
//!

GTK2.PrintBackend get_backend( );
//! Returns the backend of the printer.
//!
//!

string get_description( );
//! Gets the description of the printer.
//!
//!

string get_icon_name( );
//! Gets the name of the icon to use for the printer.
//!
//!

int get_job_count( );
//! Gets the number of jobs currently queued on the printer.
//!
//!

string get_location( );
//! Returns a description of the location of the printer.
//!
//!

string get_name( );
//! Returns the name of the printer.
//!
//!

string get_state_message( );
//! Returns the state message describing the current state of the printer.
//!
//!

int is_active( );
//! Returns whether the printer is currently active (i.e. accepts new jobs).
//!
//!

int is_default( );
//! Returns whether the printer is the default printer.
//!
//!

int is_virtual( );
//! Returns whether the printer is virtual (i.e. does not represent actual
//! printer hardware, but something like a CUPS class).
//!
//!
