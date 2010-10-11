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
//! always disable this by unsetting the GTK.CanFocus flag on the
//! scrollbar, like this:
//! 
//! @tt{scrollbar->unset_flag( GTK.CanFocus );@}
//! 
//! The key bindings (which are, of course, only active when the widget
//! has focus) are slightly different between horizontal and vertical
//! range widgets, for obvious reasons. They are also not quite the
//! same for scale widgets as they are for scrollbars, for somewhat
//! less obvious reasons (possibly to avoid confusion between the keys
//! for horizontal and vertical scrollbars in scrolled windows, where
//! both operate on the same area).
//! 
//!
//!

inherit GTK.Widget;

GTK.Adjustment get_adjustment( );
//!

int get_button( );
//!

int get_click_child( );
//!

int get_digits( );
//!

int get_in_child( );
//!

int get_need_timer( );
//!

float get_old_lower( );
//!

float get_old_page_size( );
//!

float get_old_upper( );
//!

float get_old_value( );
//!

int get_policy( );
//!

int get_scroll_type( );
//!

int get_timer( );
//!

int get_x_click_point( );
//!

int get_y_click_point( );
//!

GTK.Range set_adjustment( GTK.Adjustment pos );
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

GTK.Range set_update_policy( int when );
//! The "update policy" of a range widget defines at what points during
//! user interaction it will change the value field of its Adjustment
//! and emit the "value_changed" signal on this Adjustment. The update
//! policies are:
//! @dl
//! @item GTK.UpdatePolicyContinuous
//! This is the default. The "value_changed" signal is emitted
//! continuously, i.e., whenever the slider is moved by even the
//! tiniest amount.
//! @item GTK.UpdatePolicyDiscontinuous
//!  The "value_changed" signal is only emitted once the slider has
//! stopped moving and the user has released the mouse button.
//! @item GTK.UpdatePolicyDelayed
//! The "value_changed" signal is emitted when the user releases
//! the mouse button, or if the slider stops moving for a short period
//! of time.
//! @enddl
//!
//!
//!

GTK.Range slider_update( );
//! Update the slider values.
//!
//!
