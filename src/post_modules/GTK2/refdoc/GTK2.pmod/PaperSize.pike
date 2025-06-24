// Automatically generated from "gtkpapersize.pre".
// Do NOT edit.

//! A Paper Size.
//!
//!

protected GTK2.PaperSize _destruct( );
//! Destructor.
//!
//!

GTK2.PaperSize copy( );
//! Copy this GTK2.PaperSize.
//!
//!

protected void create( void name, void ppd_display_name, void width, void height, void unit );
//! Create a new GTK2.PaperSize object by parsing a PWG 5101.1-2002 PWG paper
//! name.
//!
//!

string get_default( );
//! Returns the name of the default paper size.
//!
//!

float get_default_bottom_margin( int unit );
//! Gets the default bottom margin.
//!
//!

float get_default_left_margin( int unit );
//! Gets the default left margin.
//!
//!

float get_default_right_margin( int unit );
//! Gets the default right margin.
//!
//!

float get_default_top_margin( int unit );
//! Gets the default top margin.
//!
//!

string get_display_name( );
//! Get the human-readable name.
//!
//!

float get_height( int unit );
//! Gets the paper height in units of unit.
//!
//!

string get_name( );
//! Get the name.
//!
//!

string get_ppd_name( );
//! Get the ppd name.  May return an empty string.
//!
//!

float get_width( int unit );
//! Gets the paper width in units of unit.
//!
//!

int is_custom( );
//! Returns 1 if this paper size is not a standard paper size.
//!
//!

int is_equal( GTK2.PaperSize size1 );
//! Comparison.
//!
//!

GTK2.PaperSize set_size( float width, float height, int unit );
//! Changes the dimensions to width x height.
//!
//!
