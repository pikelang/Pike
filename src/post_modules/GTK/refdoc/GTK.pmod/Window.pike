//! The basic window. Nothing much to say about it. It can only contain
//! one child widget. Show the main window last to avoid annoying
//! flashes when the subwidet (and it's subwidgets) are added to it,
//! this is done automatically by calling 'window->show_all' when you
//! are done with your widget packing.
//!
//!
//!  Signals:
//! @b{move_resize@}
//!
//! @b{set_focus@}
//!

inherit GTK.Bin;

int activate_default( );
//! Activate the default widget
//!
//!

int activate_focus( );
//! Activate the focus widget
//!
//!

GTK.Window add_accel_group( GTK.AccelGroup group );
//! This function adds an accelerator group to the window. The shortcuts in
//! the table will work in the window, it's child, and all children of
//! it's child that do not select keyboard input.
//!
//!

GTK.Window add_embedded_xid( int x_window_id );
//! Add an embedded X-window
//!
//!

static GTK.Window create( int window_type );
//! Argument is one of @[WINDOW_DIALOG], @[WINDOW_POPUP] and @[WINDOW_TOPLEVEL]
//!
//!

int get_allow_grow( );
//! If true, the window can grow if nessesary
//!
//!

int get_allow_shrink( );
//! If true, the window can be shrunk by the user
//!
//!

int get_auto_shrink( );
//! If true, the window will shrink if possible
//!
//!

GTK.Widget get_default_widget( );
//! The default widget
//!
//!

GTK.Widget get_focus_widget( );
//! The focus widget
//!
//!

int get_modal( );
//! If true, this is a modal dialog window
//!
//!

string get_title( );
//! The title of the window
//!
//!

GTK.Window get_transient_parent( );
//! The parent window for this window if this is a transient window, 0
//! otherwise.
//!
//!

int get_type( );
//! The window type, one of @[WINDOW_DIALOG], @[WINDOW_POPUP] and @[WINDOW_TOPLEVEL]
//!
//!

string get_wmclass_class( );
//! The window manager class of this application.
//!
//!

string get_wmclass_name( );
//! The window manager name of this application.
//!
//!

GTK.Window lower( );
//! Lower this window if the window manager allows that.
//!
//!

GTK.Window raise( );
//! Raise this window if the window manager allows that.
//!
//!

GTK.Window remove_accel_group( GTK.AccelGroup table );
//! Remove a previously installed table.
//!
//!

GTK.Window remove_embedded_xid( int x_window_id );
//! Remove the embeded X window
//!
//!

GTK.Window set_default( GTK.Widget default_widget );
//! Set the default widget to the specified widget.
//! The specified widget @b{must@} have the GTK.CanDefault flag set.
//!
//!

GTK.Window set_default_size( int width, int height );
//! The following differs from set_usize, in that
//! set_usize() overrides the requisition, and thus sets a minimum
//! size, while this only sets the size requested from the WM.
//!
//!

GTK.Window set_focus( GTK.Widget child );
//! Set the focus widget to the specified child. Please note that this
//! is normaly handled automatically.
//!
//!

GTK.Window set_icon( GDK.Pixmap p, GDK.Bitmap b, GDK.Window w );
//! Set the icon to the specified image (with mask) or the specified
//! GDK.Window. It is up to the window manager to display the icon. 
//! Most window manager handles window and pixmap icons, but only a
//! few can handle the mask argument. If you want a shaped icon, the
//! only safe bet is a shaped window.
//!
//!

GTK.Window set_icon_name( string name );
//! Set the icon name to the specified string.
//!
//!

GTK.Window set_modal( int modalp );
//!/ Is this a modal dialog?
//!
//!

GTK.Window set_policy( int allow_shrink, int allow_grow, int auto_shrink );
//! If allow shrink is true, the user can resize the window to a
//! smaller size. If allow_grow is true, the window can resize itself,
//! and the user can resize the window, to a bigger size. It auto
//! shrink is true, the window will resize itself to a smaller size
//! when it's subwidget is resized.
//!
//!

GTK.Window set_position( int pos );
//! one of @[WINDOW_DIALOG], @[WINDOW_POPUP], @[WINDOW_TOPLEVEL], @[WIN_POS_CENTER], @[WIN_POS_MOUSE] and @[WIN_POS_NONE]
//!
//!

GTK.Window set_title( string title );
//! Set the window title. The default title is the value sent to
//! setup_gtk, or if none is sent, Pike GTK.
//!
//!

GTK.Window set_transient_for( GTK.Window parent );
//! Mark this window as a transient window for the parent window.
//! Most window managers renders transient windows differently (different
//! borders, sometimes no resize widgets etc)
//! 
//! Useful for short lived dialogs.
//!
//!

GTK.Window set_wmclass( string name, string class );
//! Set the window manager application name and class.
//!
//!
