//!

inherit GTK.Widget;

GTK.Progress configure( float value, float min, float max );
//! Sets the current value, the minimum value and the maximum value.
//! The default min and max are 0.0 and 1.0 respectively.
//!
//!

int get_activity_mode( );
//! The currently set activity mode.
//!
//!

float get_current_percentage( );
//! Returns a float between 0.0 and 1.0
//!
//!

string get_current_text( );
//! Return the current text (see set_format_string)
//!
//!

string get_format( );
//! The format used to convert the value to a the text
//!
//!

float get_percentage_from_value( float value );
//! Returns a float between 0.0 and 1.0
//!
//!

int get_show_text( );
//! 1 if the text will be shown
//!
//!

string get_text_from_value( float value );
//! Formats 'value' and returns it as a text.
//!
//!

float get_value( );
//! Return the current value
//!
//!

float get_x_align( );
//! The text alignment, 0.0 is leftmost, 1.0 is rightmost
//!
//!

float get_y_align( );
//! The text alignment, 0.0 is topmost, 1.0 is bottommost
//!
//!

GTK.Progress set_activity_mode( int modep );
//! As well as indicating the amount of progress that has occured, the
//! progress bar may be set to just indicate that there is some
//! activity. This can be useful in situations where progress cannot be
//! measured against a value range.
//! Mode is:
//! 1: active
//! 0: inactive
//! 
//!
//!

GTK.Progress set_adjustment( GTK.Adjustment adjustment );
//! Sets the adjustment to use. See the adjustment documentation for more info
//!
//!

GTK.Progress set_format_string( string format );
//!
//! More or less like sprintf.
//! %[field width][character]
//! 0&lt;=width&gt;=2
//! Supported characters:
//! @pre{
//! %: Insert a %
//! p or P: The percentage completed, with 'digits' number of decimals
//! v or V: The actual value, with digits decimals.
//! l or L: The lower value (from the adjustment)
//! u or U: The higer value (from the adjustment)
//! %: Insert a %
//! @}The default format is '%P%%'
//! 
//!
//!

GTK.Progress set_percentage( float pct );
//! Sets the value (between 0.0 and 1.0). Uses the min and max values.
//!
//!

GTK.Progress set_show_text( int textp );
//! If true, write a text in the progress bar.
//!
//!

GTK.Progress set_text_alignment( float xalign, float yalign );
//! The location for the text in the progress bar.
//! xalign is between 0.0 (leftmost) and 1.0 (rightmost)
//! yalign is between 0.0 (topmost) and 1.0 (bottommost)
//! 
//! Default is xalign == yalign == 0.5
//! 
//!
//!

GTK.Progress set_value( float value );
//! Set the value.
//!
//!
