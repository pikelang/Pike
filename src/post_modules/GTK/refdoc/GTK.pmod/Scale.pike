//! The GTK.Scale widget is an abstract class, used only for deriving
//! the subclasses GTK.Hscale and GTK.Vscale.
//! 
//! See W(Range) for generic range documentation
//! 
//!
//!
inherit Range;

int get_draw_value( )
//! non-zero if the scale's current value is displayed next to the slider.
//!
//!

int get_value_pos( )
//! The position in which the textual value is displayed, selected from
//! @[POS_BOTTOM], @[POS_TOP], @[POS_RIGHT] and @[POS_LEFT]
//!
//!

int get_value_width( )
//! An internal function used to get the maximum width needed to
//! display the value string. Not normaly used by applications.
//!
//!

Scale set_digits( int precision )
//! Sets the number of decimal places that are displayed in the value.
//!
//!

Scale set_draw_value( int drawp )
//!  Specifies whether the current value is displayed as a string next
//!  to the slider.
//!
//!

Scale set_value_pos( int where )
//! Sets the position in which the current value is displayed. One of
//! @[POS_BOTTOM], @[POS_TOP], @[POS_RIGHT] and @[POS_LEFT]
//!
//!
