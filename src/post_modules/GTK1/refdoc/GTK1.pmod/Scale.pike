//! The GTK1.Scale widget is an abstract class, used only for deriving
//! the subclasses GTK1.Hscale and GTK1.Vscale.
//! 
//! See W(Range) for generic range documentation
//! 
//!
//!

inherit GTK1.Range;

int get_draw_value( );
//! non-zero if the scale's current value is displayed next to the slider.
//!
//!

int get_value_pos( );
//! The position in which the textual value is displayed, selected from
//! @[POS_BOTTOM], @[POS_LEFT], @[POS_RIGHT] and @[POS_TOP]
//!
//!

int get_value_width( );
//! An internal function used to get the maximum width needed to
//! display the value string. Not normaly used by applications.
//!
//!

GTK1.Scale set_digits( int precision );
//! Sets the number of decimal places that are displayed in the value.
//!
//!

GTK1.Scale set_draw_value( int drawp );
//!  Specifies whether the current value is displayed as a string next
//!  to the slider.
//!
//!

GTK1.Scale set_value_pos( int where );
//! Sets the position in which the current value is displayed. One of
//! @[POS_BOTTOM], @[POS_LEFT], @[POS_RIGHT] and @[POS_TOP]
//!
//!
