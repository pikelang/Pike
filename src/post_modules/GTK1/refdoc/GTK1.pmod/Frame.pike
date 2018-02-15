//! The frame widget is a Bin that surrounds its child with a
//! decorative frame and an optional label. If present, the label is
//! drawn in a gap in the top side of the frame. The position of the
//! label can be controlled with set_label_align().
//! 
//! Used to visually group objects.
//! 
//!@expr{ GTK1.Frame("Title")->add(GTK1.Label("Contents"))@}
//!@xml{<image>../images/gtk1_frame.png</image>@}
//!
//!@expr{ GTK1.Frame()->add(GTK1.Label("Contents"))@}
//!@xml{<image>../images/gtk1_frame_2.png</image>@}
//!
//!@expr{ GTK1.Frame()->add(GTK1.Label("Contents"))->set_shadow_type(GTK1.SHADOW_IN)@}
//!@xml{<image>../images/gtk1_frame_3.png</image>@}
//!
//! 
//!
//!

inherit GTK1.Container;

protected GTK1.Frame create( string|void label_text );
//! Create a new W(Frame) widget.
//!
//!

GTK1.Frame set_label( string|void label_text );
//! Set the text of the label.
//!
//!

GTK1.Frame set_label_align( float xalign, float yalign );
//! Arguments are xalignment and yalignment.
//! 0.0 is left or topmost, 1.0 is right or bottommost.
//! The default value for a newly created Frame is 0.0.
//!
//!

GTK1.Frame set_shadow_type( int shadow_type );
//! Set the shadow type for the Frame widget. The type is one of
//! @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_OUT]
//!
//!
