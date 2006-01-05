//! The Spin Button widget is generally used to allow the user to
//! select a value from a range of numeric values. It consists of a
//! text entry box with up and down arrow buttons attached to the
//! side. Selecting one of the buttons causes the value to "spin" up
//! and down the range of possible values. The entry box may also be
//! edited directly to enter a specific value.
//! 
//! The Spin Button allows the value to have zero or a number of
//! decimal places and to be incremented/decremented in configurable
//! steps. The action of holding down one of the buttons optionally
//! results in an acceleration of change in the value according to how
//! long it is depressed.
//! 
//! The Spin Button uses an W(Adjustment) object to hold information about
//! the range of values that the spin button can take.
//! 
//! The attributes of an W(Adjustment) are used by the Spin Button in the
//! following way:
//! <ul><li>value: initial value for the Spin Button</li>
//!     <li>lower: lower range value</li>
//!     <li>upper: upper range value</li>
//!     <li>step_increment: value to increment/decrement when pressing mouse button 1 on a button</li>
//!     <li>page_increment: value to increment/decrement when pressing mouse button 2 on a button</li>
//!     <li>page_size: unused</li>
//! </ul>
//! 
//! The argument order for the W(Adjustment) constructor is:
//! value, lower, upper, step_increment, page_increment, page_size
//!
//!@expr{ GTK2.SpinButton(GTK2.Adjustment(),0.1, 1 )->set_size_request(60,20)@}
//!@xml{<image>../images/gtk2_spinbutton.png</image>@}
//!
//! 
//! Properties:
//! GTK2.Adjustment adjustment
//! float climb-rate
//! int digits
//! int numeric
//! int snap-to-ticks
//! int update-policy
//! float value
//! int wrap
//! 
//! Style properties:
//! int shadow-type
//!
//!
//!  Signals:
//! @b{change_value@}
//!
//! @b{input@}
//!
//! @b{output@}
//!
//! @b{value_changed@}
//!

inherit GTK2.Entry;

GTK2.SpinButton configure( GTK2.Adjustment range, float climb_rate, int precision );
//! Adjustment (with the lower/upper/increse values), climb_rate and digits
//!
//!

static GTK2.SpinButton create( GTK2.Adjustment range_or_min_or_props, float climb_rate_or_max, int|float precision_or_step );
//!  The climb_rate argument take a value between 0.0 and 1.0 and
//!  indicates the amount of acceleration that the Spin Button has. The
//!  digits argument specifies the number of decimal places to which
//!  the value will be displayed.
//!
//!

int get_digits( );
//! Fetches the precision.
//!
//!

int get_entry( );
//! Returns W(Entry) of this widget.
//!
//!

mapping get_increments( );
//! Gets the current step and page increments.
//!
//!

int get_numeric( );
//! Returns whether non-numeric text can be typed in.
//!
//!

mapping get_range( );
//! Gets the range allowed.
//!
//!

int get_snap_to_ticks( );
//! Returns whether the value are corrected to the nearest step.
//!
//!

int get_update_policy( );
//! Gets the update behavior.
//!
//!

float get_value( );
//! Get the value.
//!
//!

int get_value_as_int( );
//! The current value of a Spin Button can be retrieved as a int.
//!
//!

int get_wrap( );
//! Returns whether the value wraps around to the opposite limit when the
//! upper or lower limit of the range is exceeded.
//!
//!

GTK2.SpinButton set_adjustment( GTK2.Adjustment range );
//! Set a new adjustment.
//!
//!

GTK2.SpinButton set_digits( int precision );
//! Set the number of digits to show to the user.
//!
//!

GTK2.SpinButton set_increments( float step, float page );
//! Sets the step and page increments.  This affects how quickly the value
//! changes when the arrows are activated.
//!
//!

GTK2.SpinButton set_numeric( int numericp );
//! If true, it is a numeric value.  This prevents a user from typing
//! anything other than numeric values into the text box of a Spin
//! Button
//!
//!

GTK2.SpinButton set_range( float min, float max );
//! Sets the minimum and maximum allowable values.
//!
//!

GTK2.SpinButton set_snap_to_ticks( int snapp );
//! Set the Spin Button to round the value to the nearest
//! step_increment, which is set within the Adjustment object used with
//! the Spin Button
//!
//!

GTK2.SpinButton set_update_policy( int policy );
//! The possible values of policy are either GTK2.UpdateAlways or
//! GTK2.UpdateIfValid.
//! 
//! These policies affect the behavior of a Spin Button when parsing
//! inserted text and syncing its value with the values of the
//! Adjustment.
//! 
//! In the case of GTK2.UpdateIfValid the Spin Button value only gets
//! changed if the text input is a numeric value that is within the
//! range specified by the Adjustment. Otherwise the text is reset
//! to the current value.
//! 
//! In case of GTK2.UpdateAlways errors are ignored while converting text
//! into a numeric value.
//!
//!

GTK2.SpinButton set_value( float to );
//! Set the value.
//!
//!

GTK2.SpinButton set_wrap( int wrapp );
//! If true, the spin button will wrap from the lowest to the highest
//! value, and the highest to the lowest.
//!
//!

GTK2.SpinButton spin( int direction, float increment );
//! If you want to alter the value of a Spin Value relative to its
//! current value, then this function can be used.
//! 
//! The direction paramenter is one of @[SPIN_END], @[SPIN_HOME], @[SPIN_PAGE_BACKWARD], @[SPIN_PAGE_FORWARD], @[SPIN_STEP_BACKWARD], @[SPIN_STEP_FORWARD] and @[SPIN_USER_DEFINED]
//! 
//!  GTK2.SpinStepForward and GTK2.SpinStepBackward change the value
//!  of the Spin Button by the amount specified by increment, unless
//!  increment is equal to 0, in which case the value is changed by the
//!  value of step_increment in theAdjustment.
//! 
//! GTK2.SpinPageForward and GTK2.SpinPageBackward simply alter the
//! value of the Spin Button by increment.
//! 
//! GTK2.SpinHome sets the value of the Spin Button to the bottom of the
//! Adjustments range.
//! 
//! GTK2.SpinEnd sets the value of the Spin Button to the top of the
//! Adjustments range.
//! 
//! GTK2.SpinUserDefined simply alters the value of the Spin Button by
//! the specified amount.
//!
//!

GTK2.SpinButton update( );
//! Explicitly request that the Spin Button updates itself
//!
//!
