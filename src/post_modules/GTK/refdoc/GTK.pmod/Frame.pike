//! The frame widget is a Bin that surrounds its child with a
//! decorative frame and an optional label. If present, the label is
//! drawn in a gap in the top side of the frame. The position of the
//! label can be controlled with set_label_align().
//! 
//! Used to visually group objects.
//! 
//!@expr{ GTK.Frame("Title")->add(GTK.Label("Contents"))@}
//!@xml{<image>../images/gtk_frame.png</image>@}
//!
//!@expr{ GTK.Frame()->add(GTK.Label("Contents"))@}
//!@xml{<image>../images/gtk_frame_2.png</image>@}
//!
//!@expr{ GTK.Frame()->add(GTK.Label("Contents"))->set_shadow_type(GTK.SHADOW_IN)@}
//!@xml{<image>../images/gtk_frame_3.png</image>@}
//!
//! 
//!
//!

inherit GTK.Container;

static GTK.Frame create( string|void label_text );
//! Create a new W(Frame) widget.
//!
//!

GTK.Frame set_label( string|void label_text );
//! Set the text of the label.
//!
//!

GTK.Frame set_label_align( float xalign, float yalign );
//! Arguments are xalignment and yalignment.
//! 0.0 is left or topmost, 1.0 is right or bottommost.
//! The default value for a newly created Frame is 0.0.
//!
//!

GTK.Frame set_shadow_type( int shadow_type );
//! Set the shadow type for the Frame widget. The type is one of
//! @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_OUT]
//!
//!
