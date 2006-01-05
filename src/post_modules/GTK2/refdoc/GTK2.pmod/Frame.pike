//! The frame widget is a Bin that surrounds its child with a
//! decorative frame and an optional label. If present, the label is
//! drawn in a gap in the top side of the frame. The position of the
//! label can be controlled with set_label_align().
//! 
//! Used to visually group objects.
//! 
//!@expr{ GTK2.Frame("Title")->add(GTK2.Label("Contents"))@}
//!@xml{<image>../images/gtk2_frame.png</image>@}
//!
//!@expr{ GTK2.Frame()->add(GTK2.Label("Contents"))@}
//!@xml{<image>../images/gtk2_frame_2.png</image>@}
//!
//!@expr{ GTK2.Frame()->add(GTK2.Label("Contents"))->set_shadow_type(GTK2.SHADOW_IN)@}
//!@xml{<image>../images/gtk2_frame_3.png</image>@}
//!
//! 
//! Properties:
//! string label
//! GTK2.Widget label-widget
//! float label-xalign
//! float label-yalign
//! int shadow
//! int shadow-type
//!
//!

inherit GTK2.Bin;

static GTK2.Frame create( string|mapping label_or_props );
//! Create a new W(Frame) widget.
//!
//!

string get_label( );
//! Return the text in the label widget.
//!
//!

mapping get_label_align( );
//! Retrieves the x and y alignment of the label.
//!
//!

GTK2.Widget get_label_widget( );
//! Retrieves the label widget.
//!
//!

int get_shadow_type( );
//! Return the shadow type.
//!
//!

GTK2.Frame set_label( string|void label_text );
//! Set the text of the label.
//!
//!

GTK2.Frame set_label_align( float xalign, float yalign );
//! Arguments are xalignment and yalignment.
//! 0.0 is left or topmost, 1.0 is right or bottommost.
//! The default value for a newly created Frame is 0.0.
//!
//!

GTK2.Frame set_label_widget( GTK2.Widget label );
//! Sets the label widget for the frame.  This is the widget that will appear
//! embedded in the top edge of the frame as a title.
//!
//!

GTK2.Frame set_shadow_type( int shadow_type );
//! Set the shadow type for the Frame widget. The type is one of
//! @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_OUT]
//!
//!
