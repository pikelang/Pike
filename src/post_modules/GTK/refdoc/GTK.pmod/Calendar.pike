//! A calendar widget.
//!@code{ GTK.Calendar();@}
//!@xml{<image src='../images/gtk_calendar.png'/>@}
//!
//!@code{ GTK.Calendar()->select_day( 16 );@}
//!@xml{<image src='../images/gtk_calendar_2.png'/>@}
//!
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
inherit Widget;

Calendar clear_marks( )
//! Remove all day markers
//!
//!

static Calendar create( )
//! Create a new calendar widget
//!
//!

Calendar display_options( int options )
//! Bitwise or of one or more of @[CALENDAR_SHOW_HEADING], @[CALENDAR_NO_MONTH_CHANGE], @[CALENDAR_SHOW_DAY_NAMES], @[CALENDAR_SHOW_WEEK_NUMBERS] and @[CALENDAR_WEEK_START_MONDAY].
//!
//!

Calendar freeze( )
//! Suspend all dynamic updating of the widget
//!
//!

mapping get_date( )
//! returns a mapping:
//! ([ "year":year, "month":month, "day":day ])
//!
//!

array(array(int)) get_day( )
//! Return an array of 6x7 days, representing the cells in the
//! currently viewed calendar month.
//!
//!

array(array(int)) get_day_month( )
//! Return an array of 6x7 days, representing the cells in the
//! currently viewed calendar month.  The value is the day of month.
//!
//!

int get_focus_col( )
//! The currently focused column
//!
//!

int get_focus_row( )
//! The currently focused row
//!
//!

int get_highlight_col( )
//! The currently highlighted column
//!
//!

int get_highlight_row( )
//! The currently highlighted row
//!
//!

array(int) get_marked_dates( )
//! Returns an array (with 31 elements) with 1es and 0es.
//!
//!

int get_month( )
//! The current month
//!
//!

int get_num_marked_dates( )
//! The number of days that are marked
//!
//!

int get_selected_day( )
//! The currently selected day
//!
//!

int get_year( )
//! The current year
//!
//!

Calendar mark_day( int day_of_month )
//! Mark a day
//!
//!

Calendar select_day( int day_of_month )
//! Select a certain day of the currently selected month
//!
//!

int select_month( int month, int year )
//! Select the month to be viewed.
//!
//!

Calendar set_marked_date_color( int index, GDK.Color color )
//! Set the color to use to mark dates
//!
//!

Calendar thaw( )
//! Resume dynamic updating of the widget
//!
//!

Calendar unmark_day( int day_of_month )
//! Unmark a day
//!
//!
