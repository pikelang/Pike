//!  A bar that GNOME applications put on the bottom of the windows to
//!  display status, progress, hints for menu items or a minibuffer for
//!  getting some sort of response. It has a stack for status messages
//!@code{ Gnome.Appbar( 1, 1, Gnome.PreferencesUser )->set_progress( 0.4 );@}
//!@xml{<image>../images/gnome_appbar.png</image>@}
//!
//!
//!
//!  Signals:
//! @b{clear_prompt@}
//! Emitted when the prompt is cleared
//!
//!
//! @b{user_response@}
//! Emitted when the user hits enter after a prompt
//!
//!

inherit GTK.Hbox;

Gnome.Appbar clear_prompt( );
//! Remove any prompt.
//!
//!

Gnome.Appbar clear_stack( );
//! Remove all status messages from appbar, and display default status
//! message (if present).
//!
//!

static Gnome.Appbar create( int has_progress, int has_status, int interactivity );
//! Create a new GNOME application status bar. If has_progress is TRUE,
//! a small progress bar widget will be created, and placed on the left
//! side of the appbar. If has_status is TRUE, a status bar, possibly
//! an editable one, is created.
//! 
//! interactivity determines whether the appbar is an interactive
//! "minibuffer" or just a status bar. If it is set to
//! Gnome.PREFERENCES_NEVER, it is never interactive. If it is set to
//! Gnome.PREFERENCES_USER we respect user preferences from
//! ui-properties. If it's Gnome.PREFERENCES_ALWAYS we are interactive
//! whether the user likes it or not. Basically, if your app supports
//! both interactive and not (for example, if you use the
//! gnome-app-util interfaces), you should use
//! Gnome.PREFERENCES_USER. Otherwise, use the setting you
//! support. Please note that "interactive" mode is not functional now;
//! GtkEntry is inadequate and so a custom widget will be written eventually.
//! 
//!
//!

GTK.Progress get_progress( );
//! Returns GTK.Progress widget pointer, so that the progress bar may
//! be manipulated further.
//!
//!

string get_response( );
//! Get the response to the prompt, if any.
//!
//!

Gnome.Appbar pop( );
//! Remove current status message, and display previous status message,
//! if any. It is OK to call this with an empty stack.
//!
//!

Gnome.Appbar push( string what );
//! Push a new status message onto the status bar stack, and display it.
//!
//!

Gnome.Appbar refresh( );
//! Reflect the current state of stack/default. Useful to force a
//! set_status to disappear.
//!
//!

Gnome.Appbar set_default( string default_status );
//! What to show when showing nothing else; defaults to "".
//!
//!

Gnome.Appbar set_progress( float percentage );
//! Sets progress bar to the given percentage. Pure sugar - with a bad
//! name, in light of the get_progress name which is not the opposite
//! of set_progress. Maybe this function should die.
//!
//!

Gnome.Appbar set_prompt( string prompt, int modal );
//! Put a prompt in the appbar and wait for a response. When the user
//! responds or cancels, a user_response signal is emitted.
//!
//!

Gnome.Appbar set_status( string status );
//! Sets the status label without changing widget state; next set or
//! push will destroy this permanently.
//!
//!
