//! The GTK2.Scale widget is an abstract class, used only for deriving
//! the subclasses GTK2.Hscale and GTK2.Vscale.
//! 
//! See W(Range) for generic range documentation
//! 
//! Properties:
//! int digits
//! int draw-value
//! int value-pos
//! 
//! Style properties:
//! int slider-length
//! int value-spacing
//!
//!
//!  Signals:
//! @b{format_value@}
//!

inherit GTK2.Range;

int get_digits( );
//! Gets the number of decimal places that are displayed.
//!
//!

int get_draw_value( );
//! Returns whether the current value is displayed as a string next to the
//! slider.
//!
//!

int get_value_pos( );
//! Gets the position in which the current value is displayed.
//!
//!

GTK2.Scale set_digits( int precision );
//! Sets the number of decimal places that are displayed in the value.
//!
//!

GTK2.Scale set_draw_value( int drawp );
//! Specifies whether the current value is displayed as a string next
//! to the slider.
//!
//!

GTK2.Scale set_value_pos( int where );
//! Sets the position in which the current value is displayed. One of
//! @[POS_BOTTOM], @[POS_LEFT], @[POS_RIGHT] and @[POS_TOP]
//!
//!
