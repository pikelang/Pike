//! A W(Frame) widget  that always maintain a specified ratio
//! between width and height. width/height == ratio
//!
//!@expr{ GTK.Aspect_frame("Title",0.5,0.5,0.4,0)->add( GTK.Label("Wrong aspect"))->set_usize(200,200)@}
//!@xml{<image>../images/gtk_aspectframe.png</image>@}
//!
//!
//!

inherit GTK.Frame;

static GTK.AspectFrame create( string label, float xalign, float yalign, float ratio, int obey_child );
//! Create a new frame. Arguments are label, xalign, yalign, ratio, obey_child
//! xalign is floats between 0 and 1, 0.0 is upper (or leftmost), 1.0 is
//! lower (or rightmost). If 'obey_child' is true, the frame will use the
//! aspect ratio of it's (one and only) child widget instead of 'ratio'.
//!
//!

GTK.AspectFrame set( float xalign, float yalign, float ratio, int obey_child );
//! Set the aspec values. Arguments are xalign, yalign, ratio, obey_child
//! xalign is floats between 0 and 1, 0.0 is upper (or leftmost), 1.0 is
//! lower (or rightmost). If 'obey_child' is true, the frame will use the
//! aspect ratio of it's (one and only) child widget instead of 'ratio'.
//!
//!
