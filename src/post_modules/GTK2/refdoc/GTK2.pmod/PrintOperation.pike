// Automatically generated from "gtkprintoperation.pre".
// Do NOT edit.

//! GTK2.PrintOperation is the high-level, portable printing API. It looks a
//! bit different than other GTK+ dialogs such as the GTK2.FileChooser, since
//! some platforms don't expose enough infrastructure to implement a good print
//! dialog. On such platforms, GTK2.PrintOperation uses the native print
//! dialog. On platforms which do not provide a native print dialog, GTK+ uses
//! its own, see GTK2.PrintUnixDialog.
//! Properties:
//! int allow-async
//! int current-page
//! GTK2.PageSetup default-page-setup
//! string export-filename
//! string job-name
//! int n-pages
//! GTK2.PrintSettings print-settings
//! int show-progress
//! GTK2.PrintStatus status
//! string status-string
//! int track-print-status
//! int unit
//! int use-full-page
//!
//!
//!  Signals:
//! @b{begin_print@}
//!
//! @b{create_custom_widget@}
//!
//! @b{custom_width_apply@}
//!
//! @b{done@}
//!
//! @b{draw_page@}
//!
//! @b{end_print@}
//!
//! @b{paginate@}
//!
//! @b{preview@}
//!
//! @b{request_page_setup@}
//!
//! @b{status_changed@}
//!

inherit G.Object;
//!

inherit GTK2.PrintOperationPreview;
//!

GTK2.PrintOperation cancel( );
//! Cancels a running print operation. This function may be called from a
//! begin-print, paginate or draw-page signal handler to stop the currently
//! running print operation.
//!
//!

protected void create( void props );
//! Create a new GTK2.PrintOperation.
//!
//!

GTK2.PageSetup get_default_page_setup( );
//! Returns the default page setup.
//!
//!

string get_error( );
//! Call this when the result of a print operation is
//! GTK2.PRINT_OPERATION_RESULT_ERROR, either as returned by run(), or in
//! the ::done signal handler.
//!
//!

GTK2.PrintSettings get_print_settings( );
//! Returns the current print settings;
//!
//!

int get_status( );
//! Returns the status of the print operation.
//!
//!

string get_status_string( );
//! Returns a string representation of the status of the print operation.
//!
//!

int is_finished( );
//! A convenience function to find out if the print operation is finished,
//! either successfully (GTK2.PRINT_STATUS_FINISHED) or unsuccessfully
//! (GTK2.PRINT_STATUS_FINISHED_ABORTED).
//!
//!

int run( int action, GTK2.Window parent );
//! Runs the print operation, by first letting the user modify print settings
//! in the print dialog, and then print the document.
//! 
//! Normally that this function does not return until the rendering of all
//! pages is complete. You can connect to the ::status-changed signal to
//! obtain some information about the progress of the print operation.
//! Furthermore, it may use a recursive mainloop to show the print dialog.
//!
//!

GTK2.PrintOperation set_allow_async( int allow_async );
//! Sets whether run() may return before the print operation is completed.
//! Note that some platforms may not allow asynchronous operation.
//!
//!

GTK2.PrintOperation set_current_page( int cur_page );
//! Sets the current page.
//! 
//! If this is called before run(), the user will be able to select to print
//! only the current page.
//!
//!

GTK2.PrintOperation set_custom_tab_label( string label );
//! Sets the label for the tab holding custom widgets.
//!
//!

GTK2.PrintOperation set_default_page_setup( GTK2.PageSetup def );
//! Makes def the default page setup.
//!
//!

GTK2.PrintOperation set_export_filename( string filename );
//! Sets up the operation to generate a file instead of showing the print
//! dialog.  The intended use of this function is for implementing
//! "Export to PDF" actions.  Currently, PDF is the only supported format.
//! 
//! "Print to PDF" support is independent of this and is done by letting the
//! user pick the "Print to PDF" item from the list of printers in the print
//! dialog.
//!
//!

GTK2.PrintOperation set_job_name( string name );
//! Sets the name of the print job.  The name is used to identify the job.
//! 
//! If you don't set a job name, GTK+ picks a default on by numbering
//! successive print jobs.
//!
//!

GTK2.PrintOperation set_n_pages( int n_pages );
//! Sets the number of pages in the documnent.
//! 
//! This must be set to a positive nubmer before the rendering starts.
//!
//!

GTK2.PrintOperation set_print_settings( GTK2.PrintSettings settings );
//! Sets the print settings.
//!
//!

GTK2.PrintOperation set_show_progress( int show_progress );
//! If true, the print operation will show a progress dialog during the
//! print operation.
//!
//!

GTK2.PrintOperation set_track_print_status( int track_status );
//! If track_status is TRUE, the print operation will try to continue report
//! on the status of the print job in the printer queues and printer. This
//! can allow your application to show things like "out of paper" issues,
//! and when the print job actually reaches the printer.
//! 
//! This function is often implemented using some form of polling, so it
//! should not be enabled unless needed.
//!
//!

GTK2.PrintOperation set_unit( int unit );
//! Sets up the transformation for the cairo context obtained from
//! GTK2.PrintContext in such a way that distances are measured in units of
//! unit.
//!
//!

GTK2.PrintOperation set_use_full_page( int full_page );
//! If full_page is true, the transformation for the cairo context obtained
//! from GTK2.PrintContext puts the origin at the top left corner of the page
//! (which may not be the top left corner of the sheet, depending on page
//! orientation and the number of pages per sheet).  Otherwise, the origin
//! is at the top left corner of the imageable area (i.e. inside the margin).
//!
//!
