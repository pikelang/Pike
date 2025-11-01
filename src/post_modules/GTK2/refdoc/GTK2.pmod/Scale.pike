// Automatically generated from "gtkscale.pre".
// Do NOT edit.

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
//!

GTK2.Scale add_mark( float value, int pos, string|void markup );
// Adds a mark at @[value].
//
// A mark is indicated visually by drawing a tick mark next to the
// scale, and GTK+ makes it easy for the user to position the scale
// exactly at the marks value.
//
// If @[markup] is specified, text is shown next to the tick mark.
//

GTK2.Scale clear_marks( );
//! Removes any marks that have been added with add_mark().
//!
//!

int get_digits( );
//! Gets the number of decimal places that are displayed.
//!
//!

int get_draw_value( );
//! Returns whether the current value is displayed as a string next to the
//! slider.
//!
//!

Pango.Layout get_layout( );
//! Gets the Pango.Layout used to display the scale.
//!
//!

mapping get_layout_offsets( );
//! Obtains the coordinates where the scale will draw the Pango.Layout
//! representing the text in the scale.  Remember when using the Pango.Layout
//! function you need to convert to and from pixels using PANGO_SCALE.
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
