//! A simple progress bar. Useful when you are doing things that take a long
//! time. Try to always have an 'abort' button whenever it makes sence.
//!@expr{ GTK.ProgressBar()->update(0.1)@}
//!@xml{<image>../images/gtk_progressbar.png</image>@}
//!
//!@expr{ GTK.ProgressBar()->set_show_text(1)->update(0.3)@}
//!@xml{<image>../images/gtk_progressbar_2.png</image>@}
//!
//!@expr{ GTK.ProgressBar()->update(0.6)@}
//!@xml{<image>../images/gtk_progressbar_3.png</image>@}
//!
//!@expr{ GTK.ProgressBar()->update(1.0)@}
//!@xml{<image>../images/gtk_progressbar_4.png</image>@}
//!
//!
//!

inherit GTK.Progress;

static GTK.ProgressBar create( );
//! Create a new progress bar.
//! The default values are:
//! Min 0.0, max 1.0, current 0.0
//!
//!

int get_activity_blocks( );
//! The number of blocks that are set.
//!
//!

int get_activity_dir( );
//! The current direction of the progress meter. 1 is forward and 0 is
//! backwards. Usefull if you for some strange reason would like to
//! know in what direction the activity indicator is swinging right
//! now...
//!
//!

int get_activity_pos( );
//! The position of the progress meter in pixels.
//!
//!

int get_activity_step( );
//! The step size of the activity indicator in pixels.
//!
//!

int get_bar_style( );
//! The progress bar style. GTK_PROGRESS_CONTINUOUS or GTK_PROGRESS_DISCRETE.
//!
//!

int get_blocks( );
//! The total number of blocks.
//!
//!

int get_orientation( );
//! The current orientation. GTK_PROGRESS_LEFT_TO_RIGHT,
//! GTK_PROGRESS_RIGHT_TO_LEFT, GTK_PROGRESS_BOTTOM_TO_TOP or
//! GTK_PROGRESS_TOP_TO_BOTTOM.
//!
//!

GTK.ProgressBar set_activity_blocks( int blocks );
//! The number of activity blocks
//!
//!

GTK.ProgressBar set_activity_step( int stepp );
//! Include activity blocks (empty gaps in the progressbar, ala windows 98)
//!
//!

GTK.ProgressBar set_bar_style( int style );
//! One of GTK.ProgressContinuous or GTK.ProgressDiscrete
//!
//!

GTK.ProgressBar set_discrete_blocks( int blocks );
//! The number of discrete blocks in the progress bar
//!
//!

GTK.ProgressBar set_orientation( int style );
//! One of
//! GTK.ProgressLeftToRight, GTK.ProgressRightToLeft,
//! GTK.ProgressBottomToTop or GTK.ProgressTopToBottom
//!
//!

GTK.ProgressBar update( float fraction );
//! 0.0 is the minimum value, 1.0 is the maximum value.
//!
//!
