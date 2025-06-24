// Automatically generated from "gtkprintsettings.pre".
// Do NOT edit.

//! A GTK2.PrintSettings object represents the settings of a print dialog
//! in a system-independent way. The main use for this object is that once
//! you've printed you can get a settings object that represents the settings
//! the user chose, and the next time you print you can pass that object in
//! so that the user doesn't have to re-set all his settings.
//! 
//! Recognized keys:
//! printer
//! orientation
//! paper-format
//! paper-width
//! paper-height
//! use-color
//! collate
//! reverse
//! duplex
//! quality
//! n-copies
//! number-up
//! resolution
//! scale
//! print-pages
//! page-ranges
//! page-set
//! default-source
//! media-type
//! dither
//! finishings
//! output-bin
//! output-file-format (PS, PDF)
//! output-uri (file://)
//! win32-driver-extra
//! win32-driver-version
//!
//!

inherit G.Object;
//!

GTK2.PrintSettings copy( );
//! Returns a copy.
//!
//!

protected void create( void props );
//! Create a new GTK2.PrintSettings.
//!
//!

string get( string key );
//! Loops up the string value associated with key.
//!
//!

int get_bool( string key );
//! Returns the boolean represented by the value that is associated with key.
//! 
//! The string "true" represents TRUE, any other string FALSE.
//!
//!

int get_collate( );
//! Gets the value of collate.
//!
//!

string get_default_source( );
//! Gets the default source.
//!
//!

string get_dither( );
//! Gets the type of dithering that is used.
//!
//!

float get_double( string key );
//! Returns the double value associated with key, or 0.0.
//!
//!

float get_double_with_default( string key, float def );
//! Returns the floating point number represented by the value that is
//! associated with key, or def if the value does not represent a floating
//! point number.
//!
//!

int get_duplex( );
//! Gets whether to print the output in duplex.
//!
//!

string get_finishings( );
//! Gets the finishings.
//!
//!

int get_int( string key );
//! Returns the integer value of key, or 0.
//!
//!

int get_int_with_default( string key, int def );
//! Returns the value of key, interpreted as an integer, or the default value.
//!
//!

float get_length( string key, int unit );
//! Returns the value associated with key, interpreted as a length.  The
//! returned value is converted to units.
//!
//!

string get_media_type( );
//! Gets the media type.
//!
//!

int get_n_copies( );
//! Gets the number of copies to print.
//!
//!

int get_number_up( );
//! Gets the number of pages per sheet.
//!
//!

int get_orientation( );
//! Get the orientation.
//!
//!

string get_output_bin( );
//! Gets the output bin.
//!
//!

array get_page_ranges( );
//! Gets the page ranges to print.
//!
//!

int get_page_set( );
//! Gets the set of pages to print.
//!
//!

float get_paper_height( int unit );
//! Gets the paper height, converted to unit.
//!
//!

GTK2.PaperSize get_paper_size( );
//! Gets the paper size.
//!
//!

float get_paper_width( int unit );
//! Gets the paper width, converted to unit.
//!
//!

int get_print_pages( );
//! Gets which pages to print.
//!
//!

string get_printer( );
//! Return printer.
//!
//!

int get_quality( );
//! Get the print quality.
//!
//!

int get_resolution( );
//! Gets the resolution in dpi.
//!
//!

int get_reverse( );
//! Gets whether to reverse the order of the pages.
//!
//!

float get_scale( );
//! Gets the scale in percent.
//!
//!

int get_use_color( );
//! Gets whether to use color.
//!
//!

int has_key( string key );
//! Returns true, if a value is associated with key.
//!
//!

GTK2.PrintSettings set( string key, string value );
//! Associates value with key.
//!
//!

GTK2.PrintSettings set_bool( string key, int value );
//! Sets key to a boolean value.
//!
//!

GTK2.PrintSettings set_collate( int collate );
//! Sets the value of collate.
//!
//!

GTK2.PrintSettings set_default_source( string def );
//! Sets the default source.
//!
//!

GTK2.PrintSettings set_dither( string dither );
//! Sets the type of dithering that is used.
//!
//!

GTK2.PrintSettings set_double( string key, float value );
//! Sets key to a double value.
//!
//!

GTK2.PrintSettings set_duplex( );
//! Sets the duplex value.
//!
//!

GTK2.PrintSettings set_finishings( string finishings );
//! Sets the finishings.
//!
//!

GTK2.PrintSettings set_int( string key, int value );
//! Sets key to an integer value.
//!
//!

GTK2.PrintSettings set_length( string key, float value, int unit );
//! Associates a length in units of unit with key.
//!
//!

GTK2.PrintSettings set_media_type( string media_type );
//! Sets the media type.
//!
//!

GTK2.PrintSettings set_n_copies( int num_copies );
//! Sets the number of copies to print.
//!
//!

GTK2.PrintSettings set_number_up( int number_up );
//! Sets the number of pages per sheet.
//!
//!

GTK2.PrintSettings set_orientation( int orientation );
//! Set orientation.
//!
//!

GTK2.PrintSettings set_output_bin( );
//! Sets the output bin.
//!
//!

GTK2.PrintSettings set_page_ranges( array page_ranges );
//! Sets the page ranges to print.
//!
//!

GTK2.PrintSettings set_page_set( int page_set );
//! Sets the set of pages to print.
//!
//!

GTK2.PrintSettings set_paper_height( float height, int unit );
//! Sets the paper height.
//!
//!

GTK2.PrintSettings set_paper_size( GTK2.PaperSize paper_size );
//! Sets the paper size.
//!
//!

GTK2.PrintSettings set_paper_width( float width, int unit );
//! Sets the paper width.
//!
//!

GTK2.PrintSettings set_print_pages( int pages );
//! Sets the pages to print mode.
//!
//!

GTK2.PrintSettings set_printer( string printer );
//! Set printer.
//!
//!

GTK2.PrintSettings set_quality( int quality );
//! Sets the print quality.
//!
//!

GTK2.PrintSettings set_resolution( int resolution );
//! Sets the resolution in dpi.
//!
//!

GTK2.PrintSettings set_reverse( int reverse );
//! Sets whether to reverse the output.
//!
//!

GTK2.PrintSettings set_scale( float scale );
//! Sets the scale in percent.
//!
//!

GTK2.PrintSettings set_use_color( int use_color );
//! Sets whether to use color.
//!
//!

GTK2.PrintSettings unset( string key );
//! Removes any value associated with key.
//!
//!
