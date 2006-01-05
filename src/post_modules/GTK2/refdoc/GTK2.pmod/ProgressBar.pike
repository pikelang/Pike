//! A simple progress bar. Useful when you are doing things that take a long
//! time. Try to always have an 'abort' button whenever it makes sence.
//!@expr{ GTK2.ProgressBar()->set_fraction(0.1)@}
//!@xml{<image>../images/gtk2_progressbar.png</image>@}
//!
//!@expr{ GTK2.ProgressBar()->set_property("show_text", 1)->set_fraction(0.3)@}
//!@xml{<image>../images/gtk2_progressbar_2.png</image>@}
//!
//!@expr{ GTK2.ProgressBar()->set_fraction(0.6)@}
//!@xml{<image>../images/gtk2_progressbar_3.png</image>@}
//!
//!@expr{ GTK2.ProgressBar()->set_fraction(1.0)@}
//!@xml{<image>../images/gtk2_progressbar_4.png</image>@}
//!
//! Properties:
//! int discrete-blocks
//! int ellipsize 
//! float fraction
//! int orientation @[PROGRESS_BOTTOM_TO_TOP], @[PROGRESS_CONTINUOUS], @[PROGRESS_DISCRETE], @[PROGRESS_LEFT_TO_RIGHT], @[PROGRESS_RIGHT_TO_LEFT] and @[PROGRESS_TOP_TO_BOTTOM]
//! float pulse-step
//! string text
//!
//!

inherit GTK2.Progress;

static GTK2.ProgressBar create( mapping|void props );
//! Create a new progress bar.
//! The default values are:
//! Min 0.0, max 1.0, current 0.0
//!
//!

int get_ellipsize( );
//! Returns the ellipsizing position of the progressbar.
//!
//!

float get_fraction( );
//! Returns the current fraction of the task that's been completed.
//!
//!

int get_orientation( );
//! Retrieves the current progress bar orientation.
//!
//!

float get_pulse_step( );
//! Retrieves the pulse step.
//!
//!

string get_text( );
//! Retrieves the text displayed superimposed on the progress bar, if any.
//!
//!

GTK2.ProgressBar pulse( );
//! Indicates that some progress is made, but you don't know how much.  Causes
//! the progress bar to enter "activity mode", where a block bounces back and
//! forth.  Each call to pulse() causes the block to move by a little bit (the
//! amount of movement per pulse is determined by set_pulse_step()).
//!
//!

GTK2.ProgressBar set_ellipsize( int mode );
//! Sets the mode used to ellipsize (add an ellipsis: "...") the text if there
//! is not enough space to render the entire string.  One of 
//! .
//!
//!

GTK2.ProgressBar set_fraction( float fraction );
//! Causes the progress bar to "fill in" the given fraction of the bar.  The
//! fraction should be between 0.0 and 1.0, inclusive.
//!
//!

GTK2.ProgressBar set_orientation( int style );
//! Causes the progress bar to switch to a different orientation
//! (left-to-right, right-to-left, top-to-bottom, or bottom-to-top).
//! One of @[PROGRESS_BOTTOM_TO_TOP], @[PROGRESS_CONTINUOUS], @[PROGRESS_DISCRETE], @[PROGRESS_LEFT_TO_RIGHT], @[PROGRESS_RIGHT_TO_LEFT] and @[PROGRESS_TOP_TO_BOTTOM].
//!
//!

GTK2.ProgressBar set_pulse_step( float fraction );
//! Sets the fraction of total progress bar length to move the bouncing block
//! for each call to pulse().
//!
//!

GTK2.ProgressBar set_text( string text );
//! Causes the given text to appear superimposed on the progress bar.
//!
//!
