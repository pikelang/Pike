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
//!@code{ GTK.SpinButton( GTK.Adjustment(),0.1, 1 )->set_usize(60,20)@}
//!@xml{<image>../images/gtk_spinbutton.png</image>@}
//!
//! 
//!
//!

inherit GTK.Entry;

GTK.SpinButton configure( GTK.Adjustment range, float climb_rate, int precision );
//! Adjustment (with the lower/upper/increse values), climb_rate and digits
//!
//!

static GTK.SpinButton create( GTK.Adjustment range, float climb_rate, int precision );
//!  The climb_rate argument take a value between 0.0 and 1.0 and
//!  indicates the amount of acceleration that the Spin Button has. The
//!  digits argument specifies the number of decimal places to which
//!  the value will be displayed.
//!
//!

float get_climb_rate( );
//! The amount of acceleration that the Spin Button has. 0.0 is no
//! accelleration and 1.0 is highest accelleration.
//!
//!

int get_digits( );
//! The number of decimal places to which the value will be displayed.
//!
//!

int get_numeric( );
//! If != 0 the user can not enter anything but numeric values.
//!
//!

int get_snap_to_ticks( );
//! If != 0 the Spin Button will round the value to the nearest
//! step_increment.
//!
//!

int get_update_policy( );
//! The update policy. GTK_UPDATE_ALWAYS or GTK_UPDATE_IF_VALID.
//!
//!

float get_value_as_float( );
//! The current value of a Spin Button can be retrieved as a float.
//!
//!

int get_value_as_int( );
//! The current value of a Spin Button can be retrieved as a int.
//!
//!

int get_wrap( );
//! If != 0 the Spin Button will wrap around between the upper and
//! lower range values.
//!
//!

GTK.SpinButton set_adjustment( GTK.Adjustment range );
//! Set a new adjustment.
//!
//!

GTK.SpinButton set_digits( int precision );
//! Set the number of digits to show to the user.
//!
//!

GTK.SpinButton set_numeric( int numericp );
//! If true, it is a numeric value.  This prevents a user from typing
//! anything other than numeric values into the text box of a Spin
//! Button
//!
//!

GTK.SpinButton set_shadow_type( int type );
//! Type is one of @[SHADOW_ETCHED_IN], @[SHADOW_ETCHED_OUT], @[SHADOW_IN], @[SHADOW_NONE] and @[SHADOW_OUT]
//!
//!

GTK.SpinButton set_snap_to_ticks( int snapp );
//! Set the Spin Button to round the value to the nearest
//! step_increment, which is set within the Adjustment object used with
//! the Spin Button
//!
//!

GTK.SpinButton set_update_policy( int policy );
//! The possible values of policy are either GTK.UpdateAlways or
//! GTK.UpdateIfValid.
//! 
//! These policies affect the behavior of a Spin Button when parsing
//! inserted text and syncing its value with the values of the
//! Adjustment.
//! 
//! In the case of GTK.UpdateIfValid the Spin Button value only gets
//! changed if the text input is a numeric value that is within the
//! range specified by the Adjustment. Otherwise the text is reset
//! to the current value.
//! 
//! In case of GTK.UpdateAlways errors are ignored while converting text
//! into a numeric value.
//!
//!

GTK.SpinButton set_value( float to );
//! Set the value.
//!
//!

GTK.SpinButton set_wrap( int wrapp );
//! If true, the spin button will wrap from the lowest to the highest
//! value, and the highest to the lowest.
//!
//!

GTK.SpinButton spin( int direction, float increment );
//! If you want to alter the value of a Spin Value relative to its
//! current value, then this ffunction can be used.
//! 
//! The direction paramenter is one of @[SPIN_END], @[SPIN_HOME], @[SPIN_PAGE_BACKWARD], @[SPIN_PAGE_FORWARD], @[SPIN_STEP_BACKWARD], @[SPIN_STEP_FORWARD] and @[SPIN_USER_DEFINED]
//! 
//!  GTK.SpinStepForward and GTK.SpinStepBackward change the value
//!  of the Spin Button by the amount specified by increment, unless
//!  increment is equal to 0, in which case the value is changed by the
//!  value of step_increment in theAdjustment.
//! 
//! GTK.SpinPageForward and GTK.SpinPageBackward simply alter the
//! value of the Spin Button by increment.
//! 
//! GTK.SpinHome sets the value of the Spin Button to the bottom of the
//! Adjustments range.
//! 
//! GTK.SpinEnd sets the value of the Spin Button to the top of the
//! Adjustments range.
//! 
//! GTK.SpinUserDefined simply alters the value of the Spin Button by
//! the specified amount.
//!
//!

GTK.SpinButton update( );
//! Explicitly request that the Spin Button updates itself
//!
//!
