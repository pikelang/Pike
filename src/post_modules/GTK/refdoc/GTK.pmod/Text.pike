//! The Text widget allows multiple lines of text to be displayed and
//! edited. It supports both multi-colored and multi-font text,
//! allowing them to be mixed in any way we wish. It also has a wide
//! set of key based text editing commands, which are compatible with
//! Emacs.
//! 
//! The text widget supports full cut-and-paste facilities, including
//! the use of double- and triple-click to select a word and a whole
//! line, respectively.
//! 
//! 
//! Bugs:<ul>
//!      <li> You cannot add text to the widget before it is realized.</li>
//!     </ul>
//! 
//!@code{ GTK.Text(GTK.Adjustment(),GTK.Adjustment())@}
//!@xml{<image>../images/gtk_text.png</image>@}
//!
//!@code{ function_object(call_out(GTK.Text(GTK.Adjustment(),GTK.Adjustment())->set_text, 0, "Some text")[0])@}
//!@xml{<image>../images/gtk_text_2.png</image>@}
//!
//!@code{ function_object(call_out(GTK.Text(GTK.Adjustment(),GTK.Adjustment())->insert, 0, "Some text", 0, GDK.Color(255,255,0), GDK.Color(0,0,0))[0])@}
//!@xml{<image>../images/gtk_text_3.png</image>@}
//!
//!
//!

inherit GTK.Editable;

GTK.Text backward_delete( int nchars );
//! Delete n characters backwards from the cursor position
//!
//!

static GTK.Text create( GTK.Adjustment xadjustment, GTK.Adjustment yadjustment );
//!  Creates a new GTK.Text widget, initialized with the given
//!  Gtk.Adjustments. These pointers can be used to track the viewing
//!  position of the GTK.Text widget. Passing NULL to either or both of
//!  them will make the text widget create it's own. You can set these
//!  later with the function gtk_text_set_adjustment()
//!
//!

GTK.Text forward_delete( int nchars );
//! Delete n characters forward from the cursor position
//!
//!

GTK.Text freeze( );
//! Freezes the widget which disallows redrawing of the widget until it
//! is thawed. This is useful if a large number of changes are going to
//! made to the text within the widget, reducing the amount of flicker
//! seen by the user.
//!
//!

int get_length( );
//! Returns the length of the all the text contained within the widget
//!
//!

int get_point( );
//! Gets the current position of the cursor as the number of characters
//! from the upper left corner of the GtkText widget.
//!
//!

string get_text( );
//! Get the current contents of the text object.
//!
//!

GTK.Text insert( string text, GDK.Font font, GDK.Color bg, GDK.Color fg );
//! syntax:
//! object insert(string what); OR
//! object insert(string what, GDK.Font font, GDK.Color fg, GDK.Color bg); OR
//! object insert(string what, 0, GDK.Color fg, GDK.Color bg); OR
//! object insert(string what, 0, GDK.Color fg); OR
//! object insert(string what, 0, 0, GDK.Color bg);
//! 
//!
//! Insert new text, optionally with colors.
//!
//!

GTK.Text set_adjustments( GTK.Adjustment xadjustment, GTK.Adjustment yadjustment );
//! Change the adjustments (as supplied to the constructor) to other
//! adjustments.
//!
//!

GTK.Text set_editable( int editablep );
//! If true, the user can change the text in the widget.
//!
//!

GTK.Text set_line_wrap( int linewrapp );
//! If true, the widget will automatically wrap the contents.
//!
//!

GTK.Text set_point( int point );
//! Sets the cursor at the given point. In this case a point
//! constitutes the number of characters from the extreme upper left
//! corner of the widget.
//!
//!

GTK.Text set_text( string to );
//! Set the text to the specified string.
//!
//!

GTK.Text set_word_wrap( int wordwrapp );
//! If true, the widget will automatically wrap the contents.
//!
//!

GTK.Text thaw( );
//! unfreeze the widget.
//!
//!
