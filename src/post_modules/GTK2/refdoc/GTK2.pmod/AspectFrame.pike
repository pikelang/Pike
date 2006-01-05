//! A W(Frame) widget  that always maintain a specified ratio
//! between width and height. width/height == ratio
//!
//!@expr{ GTK2.AspectFrame("Title",0.5,0.5,0.4,0)->add( GTK2.Label("Wrong aspect"))->set_size_request(200,200)@}
//!@xml{<image>../images/gtk2_aspectframe.png</image>@}
//!
//! Properties:
//! int obey-child
//! float ratio
//! float xalign
//! float yalign
//!
//!

inherit GTK2.Frame;

static GTK2.AspectFrame create( mapping|string label, float|void xalign, float|void yalign, float|void ratio, int|void obey_child );
//! Create a new frame. Arguments are label, xalign, yalign, ratio, obey_child
//! xalign is floats between 0 and 1, 0.0 is upper (or leftmost), 1.0 is
//! lower (or rightmost). If 'obey_child' is true, the frame will use the
//! aspect ratio of it's (one and only) child widget instead of 'ratio'.
//!
//!

GTK2.AspectFrame set( float xalign, float yalign, float ratio, int obey_child );
//! Set the aspec values. Arguments are xalign, yalign, ratio, obey_child
//! xalign is floats between 0 and 1, 0.0 is upper (or leftmost), 1.0 is
//! lower (or rightmost). If 'obey_child' is true, the frame will use the
//! aspect ratio of it's (one and only) child widget instead of 'ratio'.
//!
//!
