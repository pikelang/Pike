//! The GnomeDateEdit widget provides a way to enter dates and times
//! with a helper calendar to let the user select the date.
//!@code{ Gnome.DateEdit(time(),1,1);@}
//!@xml{<image>../images/gnome_dateedit.png</image>@}
//!
//!@code{ Gnome.DateEdit(time(),0,1);@}
//!@xml{<image>../images/gnome_dateedit_2.png</image>@}
//!
//!
//!
//!  Signals:
//! @b{date_changed@}
//!
//! @b{time_changed@}
//!

inherit GTK.Hbox;

static Gnome.DateEdit create( int the_time, int show_time, int use_24_format );
//! Creates a new GnomeDateEdit widget which can be used to provide an
//! easy to use way for entering dates and times.
//!
//!

int get_date( );
//! Returns the configured time
//!
//!

Gnome.DateEdit set_popup_range( int low_hour, int high_hour );
//! Sets the range of times that will be provide by the time popup
//! selectors.
//!
//!

Gnome.DateEdit set_time( int the_time );
//! Changes the displayed date and time in the GnomeDateEdit widget to
//! be the one represented by the_time.
//!
//!
