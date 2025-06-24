// Automatically generated from "gdkdisplay.pre".
// Do NOT edit.

//! GdkDisplay object.
//!
//!
//!  Signals:
//! @b{closed@}
//!

inherit G.Object;
//!

GDK2.Display beep( );
//! Emits a short beep on display
//!
//!

GDK2.Display close( );
//! Closes the connection to the window system for the given display, and
//! cleans up associated resources.
//!
//!

protected void create( );
//! Get the default display.
//!
//!

GDK2.Display flush( );
//! Flushes any requests queued for the windowing system; this happens
//! automatically when the main loop blocks waiting for new events, but if
//! your application is drawing without returning control to the main loop,
//! you may need to call this function explicitly.  A common case where this
//! function needs to be called is when an application is executing drawing
//! commands from a thread other than the thread where the main loop is
//! running.
//!
//!

int get_default_cursor_size( );
//! Returns the default size to use for cursors.
//!
//!

GDK2.Window get_default_group( );
//! Returns the default group leader window for all toplevel windows.  This
//! window is implicitly create by GDK.
//!
//!

GDK2.Screen get_default_screen( );
//! Get the default screen.
//!
//!

GDK2.Event get_event( );
//! Gets the next GDK2.Event to be processed, fetching events from the
//! windowing system if necessary.
//!
//!

mapping get_maximal_cursor_size( );
//! Returns the maximal size to use for cursors.
//!
//!

int get_n_screens( );
//! Gets the number of screens managed by the display.
//!
//!

string get_name( );
//! Gets the name of the display.
//!
//!

mapping get_pointer( );
//! Gets the current location of the pointer and the current modifier mask.
//!
//!

GDK2.Screen get_screen( int screen_num );
//! Returns a screen for one of the screens.
//!
//!

mapping get_window_at_pointer( );
//! Obtains the window underneath the mouse pointer, and the x,y location.
//! Returns 0 if the window is not known to GDK (for example, belongs to
//! another application).
//!
//!

GDK2.Display keyboard_ungrab( );
//! Release any keyboard grab.
//!
//!

GDK2.Display open( string name );
//! Opens a display.
//!
//!

GDK2.Event peek_event( );
//! Gets a copy of the first GDK2.Event in the event queue, without removing
//! the event from the queue.  (Note that this function will not get more
//! events from the windowing system.  It only checks the events that have
//! already been moved to the GDK event queue.)
//!
//!

int pointer_is_grabbed( );
//! Test if the pointer is grabbed.
//!
//!

GDK2.Display pointer_ungrab( );
//! Release any pointer grab.
//!
//!

GDK2.Display put_event( GDK2.Event event );
//! Appends a copy of the given event onto the front of the event queue.
//!
//!

int request_selection_notification( GDK2.Atom selection );
//! Request GdkEventOwnerChange events for ownership changes of the selection
//! named by the given atom.
//!
//!

GDK2.Display set_double_click_distance( int distance );
//! Sets the double click distance (two clicks within this distance count as
//! a double click and result in a GDK_2BUTTON_PRESS event).  See also
//! set_double_click_time().
//!
//!

GDK2.Display set_double_click_time( int msec );
//! Sets the double click time (two clicks within this time interval counts
//! as a double click and result in a GDK_2BUTTON_PRESS event).  Applications
//! should not set this, it is a global user-configured setting.
//!
//!

GDK2.Display store_clipboard( GDK2.Window clipboard_window, array targets );
//! Issues a request to the clipboard manager to store the clipboard data.
//!
//!

int supports_clipboard_persistence( );
//! Returns whether the specified display supports clipboard persistence, i.e.
//! if it's possible to store the clipboard data after an application has quit.
//! On X11 this checks if a clipboard daemon is running.
//!
//!

int supports_cursor_alpha( );
//! Returns true if cursors can use an 8bit alpha channel.  Otherwise, cursors
//! are restricted to bilevel alpha (i.e. a mask).
//!
//!

int supports_cursor_color( );
//! Returns true if multicolored cursors are supported.  Otherwise, cursors
//! have only a foreground and a background color.
//!
//!

int supports_input_shapes( );
//! Returns true if GDK2.Window->input_shape_combine_mask() can be used to
//! modify the input shape of windows.
//!
//!

int supports_selection_notification( );
//! Returns whether GdkEventOwnerChange events will be sent when the owner
//! of a selection changes.
//!
//!

int supports_shapes( );
//! Returns true if GDK2.Window->shape_combine_mask() can be used to create
//! shaped windows.
//!
//!

GDK2.Display sync( );
//! Flushes any requests queued for the windowing system and waits until all
//! requests have been handled.  This is often used for making sure that the
//! display is synchronized with the current state of the program.  Calling
//! sync() before (gdk_error_trap_pop()) makes sure that any errors
//! generated from earlier requests are handled before the error trap is
//! removed.
//! 
//! This is most useful for X11.  On windowing systems where requests are
//! handled synchronously, this function will do nothing.
//!
//!

GDK2.Display warp_pointer( GDK2.Screen screen, int x, int y );
//! Warps the pointer to the point x,y on the screen, unless the pointer
//! is confined to a window by a grab, in which case it will be moved as far
//! as allowed by the grab.  Warping the pointer creates events as if the
//! user had moved the mouse instantaneously to the destination.
//!
//!
