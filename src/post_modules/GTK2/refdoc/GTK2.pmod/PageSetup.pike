//! A GtkPageSetup object stores the page size, orientation and margins.
//! The idea is that you can get one of these from the page setup dialog
//! and then pass it to the GTK2.PrintOperation when printing. The benefit
//! of splitting this out of the GTK2.PrintSettings is that these affect
//! the actual layout of the page, and thus need to be set long before user
//! prints.
//!
//!

inherit G.Object;

GTK2.PageSetup copy( );
//! Returns a copy of this GTK2.PageSetup.
//!
//!

protected GTK2.PageSetup create( mapping|void props );
//! Create a new GTK2.PageSetup.
//!
//!

float get_bottom_margin( int unit );
//! Gets the bottom margin in units of unit.
//!
//!

float get_left_margin( int unit );
//! Gets the left margin in units of unit.
//!
//!

int get_orientation( );
//! Gets the page orientation.
//!
//!

float get_page_height( int unit );
//! Returns the page height in units of unit.
//!
//!

float get_page_width( int unit );
//! Returns the page width in units of unit.
//!
//!

float get_paper_height( int unit );
//! Returns the paper height in units of unit.
//!
//!

GTK2.PaperSize get_paper_size( );
//! Gets the paper size.
//!
//!

float get_paper_width( int unit );
//! Returns the paper width in units of unit.
//!
//!

float get_right_margin( int unit );
//! Gets the right margin in units of unit.
//!
//!

float get_top_margin( int unit );
//! Gets the top margin in units of unit.
//!
//!

GTK2.PageSetup set_bottom_margin( float margin, int unit );
//! Sets the bottom margin.
//!
//!

GTK2.PageSetup set_left_margin( float margin, int unit );
//! Sets the left margin.
//!
//!

GTK2.PageSetup set_orientation( int orientation );
//! Sets the page orientation.
//!
//!

GTK2.PageSetup set_paper_size( GTK2.PaperSize size );
//! Sets the paper size without changing the margins.
//!
//!

GTK2.PageSetup set_paper_size_and_default_margins( GTK2.PaperSize size );
//! Sets the paper size and modifies the margins.
//!
//!

GTK2.PageSetup set_right_margin( float margin, int unit );
//! Sets the right margin.
//!
//!

GTK2.PageSetup set_top_margin( float margin, int unit );
//! Sets the top margin.
//!
//!
