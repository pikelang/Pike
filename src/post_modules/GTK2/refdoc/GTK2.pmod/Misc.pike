//! The GTK2.Misc widget is an abstract widget which is not useful
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
//!@expr{ GTK2.Vbox(0,0)->add(GTK2.Label("Label"))->set_size_request(100,20)@}
//!@xml{<image>../images/gtk2_misc.png</image>@}
//!
//!@expr{ GTK2.Vbox(0,0)->add(GTK2.Label("Label")->set_alignment(1.0,0.0))->set_size_request(100,20)@}
//!@xml{<image>../images/gtk2_misc_2.png</image>@}
//!
//!@expr{ GTK2.Vbox(0,0)->add(GTK2.Label("Label")->set_alignment(0.0,0.0))->set_size_request(100,20)@}
//!@xml{<image>../images/gtk2_misc_3.png</image>@}
//!
//! Properties:
//! float xalign
//!   The horizontal alignment, from 0 (left) to 1 (right).
//! int xpad
//!   The amount of space to add on the left and right of the widget, in 
//!   pixels.
//! float yalign
//!   The vertical alignment, from 0 (top) to 1 (bottom).
//! int ypad
//!   The amount of space to add on the top and bottom of the widget, in
//!   pixels.
//!
//!

inherit GTK2.Widget;

mapping get_alignment( );
//! Gets the x and y alignment.
//!
//!

mapping get_padding( );
//! Gets the x and y padding.
//!
//!

GTK2.Misc set_alignment( float xalign, float yalign );
//! Sets the alignment of the widget.
//! 0.0 is left or topmost, 1.0 is right or bottommost.
//!
//!

GTK2.Misc set_padding( int xpad, int ypad );
//! Sets the amount of space to add around the widget. xpad and ypad
//! are specified in pixels.
//!
//!
