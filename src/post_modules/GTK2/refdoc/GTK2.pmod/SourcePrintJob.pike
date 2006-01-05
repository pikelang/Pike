//! Properties:
//! GTK2.SourceBuffer buffer
//! Gnome.PrintConfig config
//! string font
//! Pango.FontDescription font-desc
//! string header-footer-font
//! Pango.FontDescription header-footer-font-desc
//! int highlight
//! string numbers-font
//! Pango.FontDescript numbers-font-desc
//! int print-footer
//! int print-header
//! int print-numbers
//! int tabs-width
//! int wrap-mode
//!
//!
//!  Signals:
//! @b{begin_page@}
//!
//! @b{finished@}
//!

inherit G.Object;

GTK2.SourcePrintJob cancel( );
//! Cancels an asynchronous printing operation.  This will remove any pending
//! print idle handler.
//!
//!

static GTK2.SourcePrintJob create( GTK2.SourceBuffer buffer );
//! Creates a new print job object.
//!
//!

GTK2.SourceBuffer get_buffer( );
//! Gets the GTK2.SourceBuffer the print job would print.
//!
//!

int get_highlight( );
//! Determines if the job is configured to print the text highlighted with
//! colors and font styles.  Note that highlighting will happen only if the
//! buffer to print has highlighting activated.
//!
//!

int get_page( );
//! Determines the currently printing page number.
//!
//!

int get_page_count( );
//! Determines the total number of pages the job will print.  The returned
//! value is only meaningful after pagination has finished.  In practice, for
//! synchronous printing this means when "begin_page" is emitted, or after
//! print_range_async() has returned.
//!
//!

int get_print_footer( );
//! Determines if a footer is set to be printed for each page.  A footer will
//! be printed if this function returns true and some format strings have been
//! specified with set_footer_format().
//!
//!

int get_print_header( );
//! Determines if a header is set to be printed for each page.  A header will
//! be printed if this function returns true and some format strings have been
//! specified with set_header_format().
//!
//!

int get_print_numbers( );
//! Determines the interval used for line number printing.  If the value is
//! 0, no line numbers will be printed.  The default value is 1 (i.e. numbers
//! printed in all lines).
//!
//!

int get_tabs_width( );
//! Determines the configured width (in equivalent spaces) of tabulations.  The
//! default value is 8.
//!
//!

mapping get_text_margins( );
//! Determines the user set margins for the job.  The default for all four
//! margins is 0.0.
//!
//!

int get_wrap_mode( );
//! Determines the wrapping style for text lines wider than the printable
//! width.  The default is no wrapping.
//!
//!

GTK2.SourcePrintJob print( );
//! Print the document.
//!
//!

GTK2.SourcePrintJob print_range( GTK2.TextIter start, GTK2.TextIter end );
//! Similar to print(), except you can specify a range of text to print.
//! start and end can be in any order.
//!
//!

GTK2.SourcePrintJob print_range_async( GTK2.TextIter start, GTK2.TextIter end );
//! Starts to print job asynchronously.  This function will ready the job for
//! printing and install an idle handler that will render one page at a time.
//! 
//! This function will not return immediately, as only page rendering is done
//! asynchronously.  Text retrieval and paginating happens within this
//! function.  Also, if highlighting is enabled, the whole buffer needs to be
//! highlighted first.
//! 
//! To get notification when the job has finished, you must connect to the
//! "finished" signal.
//!
//!

GTK2.SourcePrintJob set_buffer( GTK2.SourceBuffer buffer );
//! Sets the GTK2.SourceBuffer the print job will print.  You need to specify
//! a buffer to print, either by the use of this function or by creating the
//! print job with create().
//!
//!

GTK2.SourcePrintJob set_footer_format( string|void left, string|void center, string|void right, int separator );
//! Like set_header_format(), but for the footer.
//!
//!

GTK2.SourcePrintJob set_header_format( string|void left, string|void center, string|void right, int separator );
//! Sets strftime like header format strings, to be printed on the left, center
//! and right of the top of each page.  The strings may include strftime(3)
//! codes which will be expanded at print time.  All strftime() codes are
//! accepted, with the addition of N for the page number and Q for the page
//! count.
//! 
//! separator specifies if a solid line should be drawn to separate the header
//! from the document text.
//! 
//! If 0 is given for any of the three arguments, that particular string will
//! not be printed.  For the header to be printed, in addition to specifying
//! format strings, you need to enable header printing with set_print_header().
//!
//!

GTK2.SourcePrintJob set_highlight( int setting );
//! Sets whether the printed text will be highlighted according to the buffer
//! rules.  Both color and font style are applied.
//!
//!

GTK2.SourcePrintJob set_print_footer( int setting );
//! Sets whether you want to print a footer in each page.  The default footer
//! consists of three pieces of text and an optional line separator,
//! configurable with set_footer_format().
//! 
//! Note that be default the footer format is unspecified, and if it's empty it
//! will not be printed, regardless of this setting.
//!
//!

GTK2.SourcePrintJob set_print_header( int setting );
//! Sets whether you want to print a header in each page.  The default header
//! consists of three pieces of text and an optional line separator,
//! configurable with set_header_format().
//! 
//! Note that by default the header format is unspecified, and if it's empty
//! it will not be printed, regardless of this setting.
//!
//!

GTK2.SourcePrintJob set_print_numbers( int interval );
//! Sets the interval for printed line numbers.  If interval is 0 no numbers
//! will be printed.  If greater than 0, a number will be printed every
//! interval lines (i.e. 1 will print all line numbers).
//!
//!

GTK2.SourcePrintJob set_tabs_width( int tabs_width );
//! Sets the width (equivalent spaces) of tabulations for the printed text.
//! The width in printing units will be calculated as the width of a string
//! containing tabs_width spaces of the default font.  Tabulation stops are
//! set for the full width of printed text.
//!
//!

GTK2.SourcePrintJob set_text_margins( float top, float bottom, float left, float right );
//! Sets the four user margins for the print job.  These margins are in
//! addition to the document margins provided in the GnomePrintConfig and will
//! not be used for headers, footers, or line numbers (those are calculated
//! separately).  You can print in the space allocate by these margins by
//! connecting to the "begin-page" signal.  The space is around the printed
//! text, and inside the margins specified in the GnomePrintConfig.
//! 
//! The margin numbers are given in device units.  If any of the given values
//! is less than 0, that particular margin is not altered by this function.
//!
//!

GTK2.SourcePrintJob set_wrap_mode( int setting );
//! Sets the wrap mode for lines of text larger than the printable width.
//!
//!

GTK2.SourcePrintJob setup_from_view( GTK2.SourceView view );
//! Convenience function to set several configuration options at once, so that
//! the printed output matches view.  The options set are buffer (if not set
//! already), tabs width, highlighting, wrap mode and default font.
//!
//!
