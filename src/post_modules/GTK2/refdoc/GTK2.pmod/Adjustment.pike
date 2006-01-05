//! The GTK2.Adjustment object represents a value which has an associated
//! lower and upper bound, together with step and page increments, and
//! a page size. It is used within several GTK2+ widgets, including
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
//! Properties:
//! float lower
//! float page-increment
//! float page-size
//! float step-increment
//! float upper
//! float value
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

inherit GTK2.Data;

GTK2.Adjustment changed( );
//! Emites a "changed" signal.
//!
//!

GTK2.Adjustment clamp_page( float lower, float upper );
//! Updates the W(Adjustment) value to ensure that the range between lower and
//! upper is in the current page (i.e. between value and value+page_size).  If
//! the range is larger than the page size, then only the start of it will be
//! in the current page.  A "changed" signal will be emitted if the value is
//! changed.
//!
//!

static GTK2.Adjustment create( float|mapping value_or_props, float|void lower, float|void upper, float|void step_increment, float|void page_increment, float|void page_size );
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

float get_value( );
//! Gets the current value.
//!
//!

GTK2.Adjustment set_value( float to );
//! Sets the value.  The value is clamped to lie between lower and upper.
//! 
//! Note that for adjustments which are used in a W(Scrollbar), the effective
//! range of allowed values goes from lower to upper-page_size.
//!
//!

GTK2.Adjustment value_changed( );
//! Emits a "value-changed" signal.
//!
//!
