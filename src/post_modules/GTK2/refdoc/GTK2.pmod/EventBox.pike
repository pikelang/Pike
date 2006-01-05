//! Some gtk widgets don't have associated X windows, so they just draw
//! on their parents. Because of this, they cannot receive events and
//! if they are incorrectly sized, they don't clip so you can get messy
//! overwritting etc. If you require more from these widgets, the
//! EventBox is for you.
//!
//! At first glance, the EventBox widget might appear to be totally
//! useless. It draws nothing on the screen and responds to no
//! events. However, it does serve a function - it provides an X window
//! for its child widget. This is important as many GTK2 widgets do not
//! have an associated X window. Not having an X window saves memory
//! and improves performance, but also has some drawbacks. A widget
//! without an X window cannot receive events, and does not perform any
//! clipping on it's contents. Although the name EventBox emphasizes
//! the event-handling function, the widget can also be used for
//! clipping.
//! 
//! The primary use for this widget is when you want to receive events
//! for a widget without a window. Examples of such widgets are labels
//! and images.
//! 
//!@expr{ GTK2.EventBox()->set_size_request(100,100)@}
//!@xml{<image>../images/gtk2_eventbox.png</image>@}
//!
//! Properties:
//! int above-child
//! int visible-window
//!
//!

inherit GTK2.Bin;

static GTK2.EventBox create( mapping|void props );
//! Create a new event box widget
//!
//!

int get_above_child( );
//! Returns whether the event box window is above or below
//! the windows of its child.
//!
//!

int get_visible_window( );
//! Returns whether the event box has a visible window.
//!
//!

GTK2.EventBox set_above_child( int above_child );
//! Sets whether the event box window is positioned above
//! the windows of its child, as opposed to below it.  If
//! the window is above, all events inside the event box
//! will go to the event box.  If the window is below, events
//! in windows of child widgets will first go to that
//! widget, and then to its parents.
//! 
//! The default is to keep the window below the child.
//!
//!

GTK2.EventBox set_visible_window( int visible_window );
//! Sets whether the event box uses a visible or invisible
//! child window.  The default is to use visible windows.
//!
//!
