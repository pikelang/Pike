//! Properties:
//! GTK2.Adjustment adjustment
//! string icons
//! int size
//! float value
//!
//!
//!  Signals:
//! @b{popdown@}
//!
//! @b{popup@}
//!
//! @b{value_changed@}
//! Scale button
//!
//!

inherit GTK2.Button;

protected GTK2.ScaleButton create( int|void size_or_props, float|void min, float|void max, float|void step, array|void icons );
//! Create a new W(ScaleButton).
//!
//!

GTK2.Adjustment get_adjustment( );
//! Returns the GTK2.Adjustment associated with this scale.
//!
//!

float get_value( );
//! Gets the current value.
//!
//!

GTK2.ScaleButton set_adjustment( GTK2.Adjustment adj );
//! Sets the GTK2.Adjustment to be used as a model.
//!
//!

GTK2.ScaleButton set_icons( array icons );
//! Sets the icons to be used.
//!
//!

GTK2.ScaleButton set_value( float val );
//! Sets the current value of the scale; if the scale is outside the minimum
//! or maximum range values, it will be clamped to fit inside them.  The
//! button emits the "value-changed" signal if the value changes.
//!
//!
