//! The GtkClock widget provides an easy way of providing a textual
//! clock in your application. It supports realtime display, as well as
//! count up and count down modes. The clock widget could conceivably
//! be used in such applications as an application's status bar, or as
//! the basis for a panel applet, etc.
//! 
//! Three modes of operation are supported. These are realtime - which
//! displays the current time, count-up/increasing - which counts up
//! from an initial value (like a stopwatch), and count-down/decreasing
//! - which counts down from an initial value.
//! 
//! Note, however, that the accuracy of the gtkclock widget is limited
//! to 1 second.
//!@code{ GTK.Clock( GTK.ClockRealtime );@}
//!@xml{<image>../images/gtk_clock.png</image>@}
//!
//!@code{ GTK.Clock( GTK.ClockDecreasing )->set_seconds(10000)->start();@}
//!@xml{<image>../images/gtk_clock_2.png</image>@}
//!
//!
//!

inherit GTK.Label;

static GTK.Clock create( int type );
//! type is one of @[CLOCK_DECREASING], @[CLOCK_INCREASING] and @[CLOCK_REALTIME];
//!
//!

GTK.Clock set_format( string fmt );
//! Set the format of a GtkClock widget. The syntax of the format
//! string is identical to that of the function strftime(3). Further
//! information about time format strings can be found on this man
//! page. The widget defaults to a format string of "%H:%M" in realtime
//! mode, or "%H:%M:%S" in count-up or count-down modes.
//!
//!

GTK.Clock set_seconds( int seconds );
//! Set the current time as displayed by the clock in count-up and
//! count-down modes. This function has no effect in realtime mode, as
//! the clock time is determined by the system clock in realtime mode.
//!
//!

GTK.Clock set_update_interval( int seconds );
//! Set the interval at which the GtkClock widget is updated. The
//! seconds parameter is used to determine how often the time shown on
//! the widget is updated. The default value is to update every second,
//! but you may wish to increase this value. If you set the update
//! interval to 0, the clock is never updated.
//!
//!

GTK.Clock start( );
//! Start the clock counting in count-up or count-down modes. The clock
//! will begin counting up or down from the time when this function is
//! called, until gtk_clock_stop is called. This function has no effect
//! in the realtime mode (you can't start and stop real time! :-).
//!
//!

GTK.Clock stop( );
//! Stop the clock counting in count-up or count-down modes. The clock
//! ceases counting up or down, and the last time reached remains on
//! the display. This function has no effect in the realtime mode (you
//! can't start and stop real time! :-).
//!
//!
