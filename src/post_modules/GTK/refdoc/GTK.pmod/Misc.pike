//! The GTK.Misc widget is an abstract widget which is not useful
//! itself, but is used to derive subclasses which have alignment and
//! padding attributes.
//! 
//! The horizontal and vertical padding attributes allows extra space
//! to be added around the widget.
//! 
//! The horizontal and vertical alignment attributes enable the widget
//! to be positioned within its allocated area. Note that if the widget
//! is added to a container in such a way that it expands automatically
//! to fill its allocated area, the alignment settings will not alter
//! the widgets position.
//! 
//!
//!@code{ GTK.Vbox(0,0)->add(GTK.Label("Label"))->set_usize(100,20)@}
//!@xml{<image>../images/gtk_misc.png</image>@}
//!
//!@code{ GTK.Vbox(0,0)->add(GTK.Label("Label")->set_alignment(1.0,0.0))->set_usize(100,20)@}
//!@xml{<image>../images/gtk_misc_2.png</image>@}
//!
//!@code{ GTK.Vbox(0,0)->add(GTK.Label("Label")->set_alignment(0.0,0.0))->set_usize(100,20)@}
//!@xml{<image>../images/gtk_misc_3.png</image>@}
//!
//!
//!

inherit GTK.Widget;

float get_xalign( );
//! The horizontal alignment, from 0 (left) to 1 (right).
//!
//!

int get_xpad( );
//! The amount of space to add on the left and right of the widget, in pixels.
//!
//!

float get_yalign( );
//! The vertical alignment, from 0 (top) to 1 (botton).
//!
//!

int get_ypad( );
//! The amount of space to add on the top and bottom of the widget, in pixels.
//!
//!

GTK.Misc set_alignment( float xalign, float yalign );
//! Sets the alignment of the widget.
//! 0.0 is left or topmost, 1.0 is right or bottommost.
//!
//!

GTK.Misc set_padding( int xpad, int ypad );
//! Sets the amount of space to add around the widget. xpand and ypad
//! are specified in pixels.
//!
//!
