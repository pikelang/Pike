//! This widget provides an analog dial widget, similar to, for
//! example, a physical volume control on a stereo. Dial values can be
//! changable or read-only for value reporting.
//!@code{ GTK.Dial( GTK.Adjustment() );@}
//!@xml{<image>../images/gtk_dial.png</image>@}
//!
//!@code{ GTK.Dial( GTK.Adjustment(10.0) )->set_percentage(0.4);@}
//!@xml{<image>../images/gtk_dial_2.png</image>@}
//!
//!
//!

inherit GTK.Widget;

static GTK.Dial create( GTK.Adjustment adjustment );
//!

GTK.Adjustment get_adjustment( );
//!

float get_percentage( );
//! Retrieves the current percentage held in the dial widget.
//!
//!

float get_value( );
//! Retrieves the current value helt in the dial widget.
//!
//!

GTK.Dial set_adjustment( GTK.Adjustment pos );
//! set_adjustment() does absolutely nothing if you pass it the
//! adjustment that range is already using, regardless of whether you
//! changed any of its fields or not. If you pass it a new Adjustment,
//! it will unreference the old one if it exists (possibly destroying
//! it), connect the appropriate signals to the new one, and call the
//! private function gtk_range_adjustment_changed(), which will (or at
//! least, is supposed to...) recalculate the size and/or position of
//! the slider and redraw if necessary.
//!
//!

GTK.Dial set_percentage( float percent );
//! Sets the GTK.Dial's value to percent of
//! dial->adjustment->upper. The upper value is set when the
//! GtkAdjustment is created.
//!
//!

GTK.Dial set_update_policy( int when );
//! The "update policy" of a range widget defines at what points during
//! user interaction it will change the value field of its Adjustment
//! and emit the "value_changed" signal on this Adjustment. The update
//! policies are:
//! <dl>
//! <dt>GTK.UpdatePolicyContinuous</dt>
//! <dd>This is the default. The "value_changed" signal is emitted
//! continuously, i.e., whenever the slider is moved by even the
//! tiniest amount.</dd>
//! <dt>GTK.UpdatePolicyDiscontinuous</dt>
//! <dd> The "value_changed" signal is only emitted once the slider has
//! stopped moving and the user has released the mouse button.</dd>
//! <dt>GTK.UpdatePolicyDelayed</dt>
//! <dd>The "value_changed" signal is emitted when the user releases
//! the mouse button, or if the slider stops moving for a short period
//! of time.</dd>
//! </dl>
//!
//!

float set_value( float to );
//! Sets the current value held in the GtkDial's adjustment object to value.
//! Returns the new percentage of value to the adjustment's upper limit.
//!
//!

GTK.Dial set_view_only( int view_only );
//! Specifies whether or not the user is to be able to edit the value
//! represented by the dial widget. If view_only is TRUE, the dial will
//! be set to view-only mode, and the user will not be able to edit
//! it. If view_only is FALSE, the user will be able to change the
//! valuerepresented.
//!
//!
