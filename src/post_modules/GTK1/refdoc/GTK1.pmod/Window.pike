//! The basic window. Nothing much to say about it. It can only contain
//! one child widget. Show the main window last to avoid annoying
//! flashes when the subwidget (and it's subwidgets) are added to it,
//! this is done automatically by calling 'window->show_all' when you
//! are done with your widget packing.
//!
//!
//!  Signals:
//! @b{move_resize@}
//!
//! @b{set_focus@}
//!

inherit GTK1.Bin;

int activate_default( );
//! Activate the default widget
//!
//!

int activate_focus( );
//! Activate the focus widget
//!
//!

GTK1.Window add_accel_group( GTK1.AccelGroup group );
//! This function adds an accelerator group to the window. The shortcuts in
//! the table will work in the window, it's child, and all children of
//! it's child that do not select keyboard input.
//!
//!

GTK1.Window add_embedded_xid( int x_window_id );
//! Add an embedded X-window
//!
//!

protected GTK1.Window create( int window_type );
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

GTK1.Widget get_default_widget( );
//! The default widget
//!
//!

GTK1.Widget get_focus_widget( );
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

GTK1.Window get_transient_parent( );
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

GTK1.Window lower( );
//! Lower this window if the window manager allows that.
//!
//!

GTK1.Window raise( );
//! Raise this window if the window manager allows that.
//!
//!

GTK1.Window remove_accel_group( GTK1.AccelGroup table );
//! Remove a previously installed table.
//!
//!

GTK1.Window remove_embedded_xid( int x_window_id );
//! Remove the embeded X window
//!
//!

GTK1.Window set_default( GTK1.Widget default_widget );
//! Set the default widget to the specified widget.
//! The specified widget @b{must@} have the GTK1.CanDefault flag set.
//!
//!

GTK1.Window set_default_size( int width, int height );
//! The following differs from set_usize, in that
//! set_usize() overrides the requisition, and thus sets a minimum
//! size, while this only sets the size requested from the WM.
//!
//!

GTK1.Window set_focus( GTK1.Widget child );
//! Set the focus widget to the specified child. Please note that this
//! is normaly handled automatically.
//!
//!

GTK1.Window set_icon( GDK1.Pixmap p, GDK1.Bitmap b, GDK1.Window w );
//! Set the icon to the specified image (with mask) or the specified
//! GDK1.Window. It is up to the window manager to display the icon. 
//! Most window manager handles window and pixmap icons, but only a
//! few can handle the mask argument. If you want a shaped icon, the
//! only safe bet is a shaped window.
//!
//!

GTK1.Window set_icon_name( string name );
//! Set the icon name to the specified string.
//!
//!

GTK1.Window set_modal( int modalp );
//!/ Is this a modal dialog?
//!
//!

GTK1.Window set_policy( int allow_shrink, int allow_grow, int auto_shrink );
//! If allow shrink is true, the user can resize the window to a
//! smaller size. If allow_grow is true, the window can resize itself,
//! and the user can resize the window, to a bigger size. It auto
//! shrink is true, the window will resize itself to a smaller size
//! when it's subwidget is resized.
//!
//!

GTK1.Window set_position( int pos );
//! one of @[WINDOW_DIALOG], @[WINDOW_POPUP], @[WINDOW_TOPLEVEL], @[WIN_POS_CENTER], @[WIN_POS_MOUSE] and @[WIN_POS_NONE]
//!
//!

GTK1.Window set_title( string title );
//! Set the window title. The default title is the value sent to
//! setup_gtk, or if none is sent, Pike GTK1.
//!
//!

GTK1.Window set_transient_for( GTK1.Window parent );
//! Mark this window as a transient window for the parent window.
//! Most window managers renders transient windows differently (different
//! borders, sometimes no resize widgets etc)
//! 
//! Useful for short lived dialogs.
//!
//!

GTK1.Window set_wmclass( string name, string class );
//! Set the window manager application name and class.
//!
//!
