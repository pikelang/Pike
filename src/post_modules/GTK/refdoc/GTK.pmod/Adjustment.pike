//! The GTK.Adjustment object represents a value which has an associated
//! lower and upper bound, together with step and page increments, and
//! a page size. It is used within several GTK+ widgets, including
//! GtkSpinButton, GtkViewport, and GtkRange (which is a base class for
//! GtkHScrollbar, GtkVScrollbar, GtkHScale, and GtkVScale).
//! 
//! The GtkAdjustment object does not update the value itself. Instead
//! it is left up to the owner of the GtkAdjustment to control the
//! value.
//! 
//! The owner of the GtkAdjustment typically calls the value_changed()
//! and changed() functions after changing the value or its
//! bounds. This results in the emission of the "value_changed" or
//! "changed" signal respectively.
//! 
//!
//!
//!  Signals:
//! @b{changed@}
//! The adjustment changed in some way
//!
//!
//! @b{value_changed@}
//! The value changed
//!
//!

inherit Data;

Adjustment changed( );
//! Call this when you have modified anything except the value member
//! of the adjustment.
//!
//!

Adjustment clamp_page( float lower, float upper );
//! Updates the GTK.Adjustment value to ensure that the range between
//! lower and upper is in the current page (i.e. between value and
//! value + page_size). If the range is larger than the page size,
//! then only the start of it will be in the current page. A
//! "value_changed" signal will be emitted if the value is changed.
//!
//!

static Adjustment create( float|void value, float|void lower, float|void upper, float|void step_increment, float|void page_increment, float|void page_size );
//! The value argument is the initial value you want to give to the
//! adjustment, usually corresponding to the topmost or leftmost
//! position of an adjustable widget. The lower argument specifies the
//! lowest value which the adjustment can hold. The step_increment
//! argument specifies the "smaller" of the two increments by which
//! the user can change the value, while the page_increment is the
//! "larger" one. The page_size argument usually corresponds somehow
//! to the visible area of a panning widget. The upper argument is
//! used to represent the bottom most or right most coordinate in a
//! panning widget's child. Therefore it is not always the largest
//! number that value can take, since the page_size of such widgets is
//! usually non-zero.
//!
//! All values are optional, they default to 0.0.
//! For most widgets the unit is pixels.
//!
//!

float get_lower( );
//! Get the lower limit
//!
//!

float get_page_increment( );
//! Get the page increment (page down or through click)
//!
//!

float get_page_size( );
//! Get the page size (the actual size of a page)
//!
//!

float get_step_increment( );
//! Get the step increment (arrow click)
//!
//!

float get_upper( );
//! get the upper limit.
//!
//!

float get_value( );
//! Get the value component.
//!
//!

float set_set_lower( );
//! Set the lower limit.
//!
//!

float set_set_page_increment( );
//! Set the page increment (page down or through click)
//!
//!

float set_set_page_size( );
//! Set the page size (the actual size of a page)
//!
//!

float set_set_step_increment( );
//! Set the step increment (arrow click)
//!
//!

float set_set_upper( );
//! Set the upper limit.
//!
//!

Adjustment set_value( float to );
//! Set the value component.
//!
//!
