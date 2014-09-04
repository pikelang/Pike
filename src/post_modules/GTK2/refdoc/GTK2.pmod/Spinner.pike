//! A @@[Spinner] widget displays an icon-size spinning animation. It
//! is often used as an alternative to a GtkProgressBar for displaying
//! indefinite activity, instead of actual progress.
//!
//! To start the animation, use @@[start], to stop it use @@[stop]
//! Properties:
//! int active
//!
//!
//!

inherit GTK2.DrawingArea;

protected GTK2.Spinner create( );
//! Start the spinner
//!
//!

GTK2.Spinner start( );
//! Stop the spinner
//!
//!

GTK2.Spinner stop( );
//!
