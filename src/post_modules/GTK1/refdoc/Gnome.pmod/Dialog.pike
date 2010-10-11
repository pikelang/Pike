//! Gnome.Dialog gives dialogs a consistent look and feel, while making
//! them more convenient to program. Gnome.Dialog makes it easy to use
//! stock buttons, makes it easier to handle delete_event, and adds
//! some cosmetic touches (such as a separator above the buttons, and a
//! bevel around the edge of the window).
//!
//!
//!  Signals:
//! @b{clicked@}
//!
//! @b{close@}
//!

inherit GTK.Window;

Gnome.Dialog append_button_with_pixmap( string name, string pixmap_file );
//!

Gnome.Dialog button_connect( int button, function callback_cb, mixed callback_arg );
//! Simply a signal_connect to the "clicked" signal of the specified button.
//!
//!

Gnome.Dialog close( );
//! See also close_hides(). This function emits the "close" signal(
//! which either hides or destroys the dialog (destroy by default). If
//! you connect to the "close" signal, and your callback returns TRUE,
//! the hide or destroy will be blocked. You can do this to avoid
//! closing the dialog if the user gives invalid input, for example.
//! 
//! Using close() in place of hide() or destroy() allows you to easily
//! catch all sources of dialog closure, including delete_event and
//! button clicks, and handle them in a central location. 
//!
//!

static Gnome.Dialog create( string title, string... buttons );
//! Creates a new Gnome.Dialog, with the given title, and any button
//! names in the arg list. Buttons can be simple names, such as "My
//! Button", or gnome-stock defines such as GNOME.StockButtonOK,
//! etc. The last argument should be NULL to terminate the list.
//! 
//! Buttons passed to this function are numbered from left to right,
//! starting with 0. So the first button in the arglist is button 0,
//! then button 1, etc. These numbers are used throughout the
//! Gnome.Dialog API.
//! 
//!
//!

Gnome.Dialog editable_enters( GTK.Editable widget );
//! Normally if there's an editable widget (such as GtkEntry) in your
//! dialog, pressing Enter will activate the editable rather than the
//! default dialog button. However, in most cases, the user expects to
//! type something in and then press enter to close the dialog. This
//! function enables that behavior.
//!
//!

GTK.Vbox get_vbox( );
//!

int run( );
//! Blocks until the user clicks a button or closes the dialog with
//! the window manager's close decoration (or by pressing Escape).
//! 
//! You need to set up the dialog to do the right thing when a button
//! is clicked or delete_event is received; you must consider both of
//! those possibilities so that you know the status of the dialog when
//! run() returns. A common mistake is to forget about Escape and the
//! window manager close decoration; by default, these call close(),
//! which by default destroys the dialog. If your button clicks do not
//! destroy the dialog, you don't know whether the dialog is destroyed
//! when run() returns. This is bad.
//! 
//! So you should either close the dialog on button clicks as well, or
//! change the close() behavior to hide instead of destroy. You can do
//! this with close_hides().
//! 
//!
//!

int run_and_close( );
//! See run(). The only difference is that this function calls close()
//! before returning if the dialog was not already closed.
//!
//!

Gnome.Dialog set_accelerator( int button, int accelerator_key, int accelerator_mode );
//!

Gnome.Dialog set_close( int click_closes );
//! This is a convenience function so you don't have to connect
//! callbacks to each button just to close the dialog. By default,
//! Gnome.Dialog has this parameter set the FALSE and it will not close
//! on any click. (This was a design error.) However, almost all the
//! Gnome.Dialog subclasses, such as Gnome.MessageBox and
//! Gnome.PropertyBox, have this parameter set to TRUE by default.
//!
//!

Gnome.Dialog set_default( int button );
//! The default button will be activated if the user just presses
//! return. Usually you should make the least-destructive button the
//! default. Otherwise, the most commonly-used button.
//!
//!

Gnome.Dialog set_parent( GTK.Window parent );
//! Dialogs have "parents," usually the main application window which
//! spawned them. This function will let the window manager know about
//! the parent-child relationship. Usually this means the dialog must
//! stay on top of the parent, and will be minimized when the parent
//! is. Gnome also allows users to request dialog placement above the
//! parent window (vs. at the mouse position, or at a default window
//! manger location).
//!
//!

Gnome.Dialog set_sensitive( int button, int sensitive );
//! Calls set_sensitive() on the specified button number.
//!
//!
