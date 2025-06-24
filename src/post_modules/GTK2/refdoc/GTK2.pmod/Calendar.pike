// Automatically generated from "gtkcalendar.pre".
// Do NOT edit.

//! A calendar widget.
//!@expr{ GTK2.Calendar();@}
//!@xml{<image>../images/gtk2_calendar.png</image>@}
//!
//!@expr{ GTK2.Calendar()->select_day( 16 );@}
//!@xml{<image>../images/gtk2_calendar_2.png</image>@}
//!
//! Properties:
//! int day
//! int month
//! int no-month-change
//! int show-day-names
//! int show-heading
//! int show-week-numbers
//! int year
//!
//!
//!  Signals:
//! @b{day_selected@}
//!
//! @b{day_selected_double_click@}
//!
//! @b{month_changed@}
//!
//! @b{next_month@}
//!
//! @b{next_year@}
//!
//! @b{prev_month@}
//!
//! @b{prev_year@}
//!

inherit GTK2.Widget;
//!

GTK2.Calendar clear_marks( );
//! Remove all day markers
//!
//!

protected void create( void props );
//! Create a new calendar widget
//!
//!

GTK2.Calendar freeze( );
//! Suspend all dynamic updating of the widget
//!
//!

mapping get_date( );
//! returns a mapping:
//! ([ "year":year, "month":month, "day":day ])
//!
//!

int get_display_options( );
//! Returns the current display options.
//!
//!

array get_marked_dates( );
//! Returns an array (with 31 elements) with 1es and 0es.
//!
//!

GTK2.Calendar mark_day( int day_of_month );
//! Mark a day
//!
//!

GTK2.Calendar select_day( int day_of_month );
//! Select a certain day of the currently selected month
//!
//!

int select_month( int month, int year );
//! Select the month to be viewed.
//!
//!

GTK2.Calendar set_display_options( int options );
//! Bitwise or of one or more of @[CALENDAR_NO_MONTH_CHANGE], @[CALENDAR_SHOW_DAY_NAMES], @[CALENDAR_SHOW_HEADING], @[CALENDAR_SHOW_WEEK_NUMBERS] and @[CALENDAR_WEEK_START_MONDAY].
//!
//!

GTK2.Calendar thaw( );
//! Resume dynamic updating of the widget
//!
//!

GTK2.Calendar unmark_day( int day_of_month );
//! Unmark a day
//!
//!
