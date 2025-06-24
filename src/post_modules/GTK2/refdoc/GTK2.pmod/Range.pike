// Automatically generated from "gtkrange.pre".
// Do NOT edit.

//! The category of range widgets includes the ubiquitous scrollbar
//! widget and the less common "scale" widget. Though these two types
//! of widgets are generally used for different purposes, they are
//! quite similar in function and implementation. All range widgets
//! share a set of common graphic elements, each of which has its own X
//! window and receives events. They all contain a "trough" and a
//! "slider" (what is sometimes called a "thumbwheel" in other GUI
//! environments). Dragging the slider with the pointer moves it back
//! and forth within the trough, while clicking in the trough advances
//! the slider towards the location of the click, either completely, or
//! by a designated amount, depending on which mouse button is used.
//! 
//! As mentioned in the W(Adjustment) page, all range widgets are
//! associated with an adjustment object, from which they calculate the
//! length of the slider and its position within the trough. When the
//! user manipulates the slider, the range widget will change the value
//! of the adjustment.
//! 
//! All of the GTK range widgets react to mouse clicks in more or less
//! the same way. Clicking button-1 in the trough will cause its
//! adjustment's page_increment to be added or subtracted from its
//! value, and the slider to be moved accordingly. Clicking mouse
//! button-2 in the trough will jump the slider to the point at which
//! the button was clicked. Clicking any button on a scrollbar's arrows
//! will cause its adjustment's value to change step_increment at a
//! time.
//! 
//! It may take a little while to get used to, but by default,
//! scrollbars as well as scale widgets can take the keyboard focus in
//! GTK. If you think your users will find this too confusing, you can
//! always disable this by unsetting the GTK2.CanFocus flag on the
//! scrollbar, like this:
//! 
//! @tt{scrollbar->unset_flag(GTK2.CanFocus);@}
//! 
//! The key bindings (which are, of course, only active when the widget
//! has focus) are slightly different between horizontal and vertical
//! range widgets, for obvious reasons. They are also not quite the
//! same for scale widgets as they are for scrollbars, for somewhat
//! less obvious reasons (possibly to avoid confusion between the keys
//! for horizontal and vertical scrollbars in scrolled windows, where
//! both operate on the same area).
//! 
//! Properties:
//! GTK2.Adjustment adjustment
//! float fill-level
//! int inverted
//! int restrict-to-fill-level
//! int show-fill-level
//! int update-policy
//! 
//! Style properties:
//! int arrow-displacement-x
//! int arrow-displacement-y
//! int slider-width
//! int stepper-size
//! int stepper-spacing
//! int trough-border
//! int trough-side-details
//! int trough-under-steppers
//!
//!
//!  Signals:
//! @b{adjust_bounds@}
//!
//! @b{change_value@}
//!
//! @b{move_slider@}
//!
//! @b{value_changed@}
//!

inherit GTK2.Widget;
//!

GTK2.Adjustment get_adjustment( );
//! Gets the W(Adjustment) which is the "model" object for W(Range).
//!
//!

float get_fill_level( );
//! Gets the current position of the fill level indicator.
//!
//!

int get_inverted( );
//! Gets the value set by set_inverted().
//!
//!

GTK2.Range get_lower_stepper_sensitivity( );
//! Gets the sensitivity policy for the stepper that points to the 'lower' end
//! of the GTK2.Range's adjustment.
//!
//!

int get_restrict_to_fill_level( );
//! Gets whether the range is restricted to the fill level.
//!
//!

int get_show_fill_level( );
//! Gets whether the range displays the fill level graphically.
//!
//!

int get_update_policy( );
//! Gets the update policy.
//!
//!

GTK2.Range get_upper_stepper_sensitivity( );
//! Gets the sensitivity policy for the stepper that points to the 'upper' end
//! of the GTK2.Range's adjustment.
//!
//!

float get_value( );
//! Gets the current value.
//!
//!

GTK2.Range set_adjustment( GTK2.Adjustment pos );
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

GTK2.Range set_fill_level( float fill );
//! Set the new position of the fill level indicator.
//!
//!

GTK2.Range set_increments( float step, float page );
//! Sets the step and page sizes.  The step size is used when the user clicks
//! the W(Scrollbar) arrows or moves W(Scale) via arrow keys.  The page size
//! is used for example when moving via Page Up or Page Down keys.
//!
//!

GTK2.Range set_inverted( int setting );
//! Ranges normally move from lower to higher values as the slider moves from
//! top to bottom or left to right.  Inverted ranges have higher values at the
//! top or on the right rather than on the bottom or left.
//!
//!

GTK2.Range set_lower_stepper_sensitivity( int sensitivity );
//! Sets the sensitivity policy for the stepper that points to the 'lower' end
//! of the GTK2.Range's adjustment.
//!
//!

GTK2.Range set_range( float min, float max );
//! Sets the allowable values, and clamps the range value to be between min
//! and max.
//!
//!

GTK2.Range set_restrict_to_fill_level( int rest );
//! Sets whether the slider is restricted to the fill level
//!
//!

GTK2.Range set_show_fill_level( int show );
//! Sets whether a graphical fill level is show on the trough.
//!
//!

GTK2.Range set_update_policy( int when );
//! The "update policy" of a range widget defines at what points during
//! user interaction it will change the value field of its Adjustment
//! and emit the "value_changed" signal on this Adjustment. The update
//! policies are:
//! @dl
//! @item GTK2.UpdatePolicyContinuous
//! This is the default. The "value_changed" signal is emitted
//! continuously, i.e., whenever the slider is moved by even the
//! tiniest amount.
//! @item GTK2.UpdatePolicyDiscontinuous
//!  The "value_changed" signal is only emitted once the slider has
//! stopped moving and the user has released the mouse button.
//! @item GTK2.UpdatePolicyDelayed
//! The "value_changed" signal is emitted when the user releases
//! the mouse button, or if the slider stops moving for a short period
//! of time.
//! @enddl
//!
//!
//!

GTK2.Range set_upper_stepper_sensitivity( int sensitivity );
//! Sets the sensitivity policy for the stepper that points to the 'upper' end
//! of the GTK2.Range's adjustment.
//!
//!

GTK2.Range set_value( float value );
//! Sets the current value; if the value is outside the minimum or maximum
//! range values, it will be clamped to fit inside them.  The range emits the
//! "value-changed" signal if the value changes.
//!
//!
