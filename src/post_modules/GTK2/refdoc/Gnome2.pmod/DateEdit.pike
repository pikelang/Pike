// Automatically generated from "gnomedateedit.pre".
// Do NOT edit.

//! The GnomeDateEdit widget provides a way to enter dates and times
//! with a helper calendar to let the user select the date.
//!@expr{ GTK2.Gnome2DateEdit(time(),1,1);@}
//!@xml{<image>../images/gnome2_dateedit.png</image>@}
//!
//!@expr{ GTK2.Gnome2DateEdit(time(),0,1);@}
//!@xml{<image>../images/gnome2_dateedit_2.png</image>@}
//!
//! Properties:
//! int dateedit-flags
//! int initial-time
//! int lower-hour
//! int time
//! int upper-hour
//!
//!
//!  Signals:
//! @b{date_changed@}
//!
//! @b{time_changed@}
//!

inherit GTK2.Hbox;
//!

protected void create( void the_time, void show_time, void use_24_format );
//! Creates a new GnomeDateEdit widget which can be used to provide an
//! easy to use way for entering dates and times.
//!
//!

int get_flags( );
//! Get the flags.
//!
//!

int64 get_initial_time( );
//! Queries the initial time that was set using set_time() or during creation.
//!
//!

int64 get_time( );
//! Return the time entered in the widget.
//!
//!

Gnome2.DateEdit set_flags( int flags );
//! Bitwise or of @[GNOME_DATE_EDIT_24_HR], @[GNOME_DATE_EDIT_SHOW_TIME] and @[GNOME_DATE_EDIT_WEEK_STARTS_ON_MONDAY].
//!
//!

Gnome2.DateEdit set_popup_range( int low_hour, int up_hour );
//! Sets the range of times that will be provide by the time popup
//! selectors.
//!
//!

Gnome2.DateEdit set_time( int64 the_time );
//! Changes the displayed date and time in the GnomeDateEdit widget to
//! be the one represented by the_time.
//!
//!
