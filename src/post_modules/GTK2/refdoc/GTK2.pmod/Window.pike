//! The basic window. Nothing much to say about it. It can only contain
//! one child widget. Show the main window last to avoid annoying
//! flashes when the subwidget (and it's subwidgets) are added to it,
//! this is done automatically by calling 'window->show_all' when you
//! are done with your widget packing.
//! Properties:
//! int accept-focus
//! int allow-grow
//! int allow-shrink
//! int decorated
//! int default-height
//! int default-width
//! int destroy-with-parent
//! int focus-on-map
//! int has-toplevel-focus
//! GDK2.Pixbuf icon
//! int is-active
//! int modal
//! int resizable
//! string role
//! GDK2.Screen screen
//! int skip-pager-hint
//! int skip-taskbar-hint
//! string title
//! int type @[WINDOW_POPUP] and @[WINDOW_TOPLEVEL]
//! int type-hint @[GDK_WINDOW_TYPE_HINT_DIALOG], @[GDK_WINDOW_TYPE_HINT_MENU], @[GDK_WINDOW_TYPE_HINT_NORMAL] and @[GDK_WINDOW_TYPE_HINT_TOOLBAR]
//! int window-position @[WIN_POS_CENTER], @[WIN_POS_CENTER_ALWAYS], @[WIN_POS_CENTER_ON_PARENT], @[WIN_POS_MOUSE] and @[WIN_POS_NONE]
//!
//!
//!  Signals:
//! @b{activate_default@}
//!
//! @b{activate_focus@}
//!
//! @b{frame_event@}
//!
//! @b{keys_changed@}
//!
//! @b{move_focus@}
//!
//! @b{set_focus@}
//!

inherit GTK2.Bin;

int activate_default( );
//! Activates the default widget, unless the current focused widget has been
//! configured to receive the default action, in which case the focuses widget
//! is activated.
//!
//!

int activate_focus( );
//! Activates the current focused widget.
//!
//!

GTK2.Window add_accel_group( GTK2.AccelGroup group );
//! This function adds an accelerator group to the window. The shortcuts in
//! the table will work in the window, it's child, and all children of
//! it's child that do not select keyboard input.
//!
//!

GTK2.Window add_mnemonic( int keyval, GTK2.Widget target );
//! Adds a mnemonic to this window.
//!
//!

static GTK2.Window create( int|mapping props );
//! Argument is one of @[WINDOW_POPUP] and @[WINDOW_TOPLEVEL], or a mapping of allowed properties.
//!
//!

GTK2.Window deiconify( );
//! Opposite of iconify().
//!
//!

int get_decorated( );
//! Returns whether the window has been set to have decorations such as a title
//! bar.
//!
//!

mapping get_default_size( );
//! Gets the default size of the window.  A value of -1 for the width or height
//! indicates that a default size has not been explicitly set for that
//! dimeension, so the "natural" size of the window will be used.
//!
//!

int get_destroy_with_parent( );
//! Returns whether the window will be destroyed with its transient parent.
//!
//!

GTK2.Widget get_focus( );
//! Retrieves the current focused widget within the window.  Note that this is
//! the widget that would have the focus if the toplevel window focused; if the
//! toplevel window is not focused then has_focus() will not be true for the
//! widget.
//!
//!

GTK2.Window get_gravity( );
//! Gets the value set by set_gravity().
//!
//!

GTK2.GdkPixbuf get_icon( );
//! Gets the value set by set_icon().
//!
//!

array get_icon_list( );
//! Retrieves the list of icons set by set_icon_list().
//!
//!

int get_mnemonic_modifier( );
//! Returns the mnemonic modifier for this window.
//!
//!

int get_modal( );
//! Returns whether the window is modal.
//!
//!

mapping get_position( );
//! This function returns the position you need to pass to move() to keep this
//! window in its current position.  This means that the meaning of the
//! returned value varies with window gravity.
//! 
//! If you haven't changed the window gravity, its gravity will be
//! GDK2.GRAVITY_NORTH_WEST.  This means that get_position() gets the position
//! of the top-left corner of the window manager frame for the window.  move()
//! sets the position of this same top-left corner.
//! 
//! get_position() is not 100% reliable because X does not specify a way to
//! obtain the geometry of the decorations placed on a window by the window
//! manager.  Thus GTK+ is using a "best guess" that works with most window
//! managers.
//! 
//! Moreover, nearly all window managers are historically broken with respect
//! to their handling of window gravity.  So moving a window to its current
//! position as returned by get_position() tends to result in moving the window
//! slightly.  Window managers are slowly getting better over time.
//! 
//! If a window has gravity GDK2.GRAVITY_STATIC the window manager frame is not
//! relevant, and thus get_position() will always produc accurate results.
//! However you can't use static gravity to do things like place a window in a
//! corner of the screen, because static gravity ignores the window manager
//! decorations.
//! 
//! If you are saving and restoring your application's window positions, you
//! should know that it's impossible for applications to do this without
//! getting it somewhat wrong because applications do not have sufficient
//! knowledge of window manager state.
//!
//!

GTK2.Window get_resizable( );
//! Gets the whether this window is resizable.
//!
//!

mapping get_size( );
//! Obtains the current size of window.
//!
//!

string get_title( );
//! Retrieves the title of the window.
//!
//!

GTK2.Window get_transient_for( );
//! Fetches the transient parent for this window.
//!
//!

int get_type_hint( );
//! Gets the type hint.
//!
//!

GTK2.Window iconify( );
//! Asks to iconify (minimize) this window.  Note that you shouldn't
//! assume the window is definitely iconified afterward, because other
//! entities (e.g. the user or window manager) could deiconify it again,
//! or there may not be a window manager in which case iconification isn't
//! possible, etc.
//!
//!

GTK2.Window lower( );
//! Lower this window if the window manager allows that.
//!
//!

GTK2.Window maximize( );
//! Maximize a window.  Same caveats as iconify().
//!
//!

int mnemonic_activate( int keyval, int modifier );
//! Activates the targets associated with the mnemonic.  modifier is one of
//! GDK2.SHIFT_MASK, GDK2.LOCK_MASK, GDK2.CONTROL_MASK, GDK2.MOD1_MASK,
//! GDK2.MOD2_MASK, GDK2.MOD3_MASK, GDK2.MOD4_MASK, GDK2.MOD5_MASK, 
//! GDK2.BUTTON1_MASK, GDK2.BUTTON2_MASK, GDK2.BUTTON3_MASK, GDK2.BUTTON4_MASK
//! GDK2.BUTTON5_MASK, GDK2.RELEASE_MASK, GDK2.MODIFIER_MASK
//!
//!

GTK2.Window move( int x, int y );
//! Asks the window manage to move the window to the given position.  Window
//! managers are free to ignore this; most window managers ignore request for
//! initial window positions (instead using a user-defined placement algorithm)
//! and honor requests after the window has already been shown.
//! 
//! Note: the position is the position of the gravity-determined reference
//! point for the window.  The gravity determines two things: first, the
//! location of the reference point in root window coordinates; and second,
//! which point on the window is position at the reference point.
//! 
//! By default the gravity is GDK2.GRAVITY_NORTH_WEST, so the reference point is
//! simply the x,y supplied to move().  The top-left corner of the window
//! decorations (aka window frame or border) will be place at x,y.  Therefore,
//! to position a window at the top left of the screen, you want to use the
//! default gravity (which is GDK2.GRAVITY_NORTH_WEST) and move the window to
//! 0,0.
//! 
//! To position a window at the bottom right corner of the screen, you would
//! set GDK2.GRAVITY_SOUTH_EAST, which means that the reference point is at
//! x + the window width and y + the window height, and the bottom-right corner
//! of the window border will be placed at that reference point.
//!
//!

int parse_geometry( string geometry );
//! Parses a standard X geometry string.
//!
//!

GTK2.Window present( );
//! Presents this window to the user.  This may mean raising the window
//! in the stacking order, deiconifying it, moving it to the current
//! desktop, and/or giving it the keyboard focus, possibly dependent
//! on the user's platform, window manager, and preferences.
//! 
//! If hidden, it calls show() as well.
//!
//!

GTK2.Window raise( );
//! Raise this window if the window manager allows that.
//!
//!

GTK2.Window remove_accel_group( GTK2.AccelGroup group );
//! Reverses the effects of add_accel_group().
//!
//!

GTK2.Window remove_mnemonic( int keyval, GTK2.Widget target );
//! Removes a mnemonic from this window.
//!
//!

GTK2.Window reshow_with_initial_size( );
//! Hides the window, then reshows it, resetting the default size and position
//! of the window.
//!
//!

GTK2.Window resize( int width, int height );
//! Resizes the window as if the user had done so, obeying geometry 
//! constraints.
//!
//!

GTK2.Window set_decorated( int setting );
//! Add title bar, resize controls, etc.
//! Default is true, so you may only need to use this function if
//! you want to remove decorations.  Depending on the system, this
//! function may not have any effect when called on a window that is
//! already visible, so you should call it before calling show().
//!
//!

GTK2.Window set_default( GTK2.Widget child );
//! The default widget is the widget that's activated when the user presses
//! Enter in a dialog (for example).  This function sets or unsets the default
//! widget.  When setting (rather than unsetting) the default widget it's
//! generally easier to call GTK2.Widget->grab_focus() on the widget.
//!
//!

GTK2.Window set_default_size( int width, int height );
//! Sets the default size of a window.  If the window's "natural" size (its
//! size request) is larger than the default, the default will be ignored.
//! more generally, if the default size does not obey the geometry hints for
//! the window (set_geometry_hints() can be used to set these explicitly), the
//! default size will be clamped to the nearest permitted size.
//! 
//! Unlike set_size_request(), which sets a size request for a widget and thus
//! would keep users from shrinking the window, this function only sets the
//! initial size, just as if the user had resized the window themselves.  Users
//! can still shrink the window again as they normally would.  Setting a
//! default size of -1 means to use the "natural" default size (the size
//! request of the window).
//! 
//! For more control over a window's initial size and how resizing works,
//! investigate set_geometry_hints().
//! 
//! For some uses, resize() is a more appropriate function.  resize() changes
//! the current size of the window, rather than the size to be used on initial
//! display.  resize() always affects the window itself, not the geometry
//! widget.
//! 
//! The default size of a window only affects the first time a window is shown;
//! if a window is hidden and re-shown, it will remember the size it had prior
//! to hiding, rather than using the default size.
//! 
//! Window scan't actually be 0x0 in size, they must be at least 1x1, but
//! passing 0 for width and height is OK, resulting in a 1x1 default size.
//!
//!

GTK2.Window set_destroy_with_parent( int setting );
//! If setting is true, then destroying the transient parent of this window
//! will also destroy the window itself.  This is useful for dialogs that
//! shouldn't persist beyond the lifetime of the main window they're associated
//! with, for example.
//!
//!

GTK2.Window set_focus( GTK2.Widget child );
//! If child is not the current focus widget, and is focusable, sets it as the
//! focus widget for the window.  If child is 0, unsets the focus widget for
//! this window.  To set the focus to a particular widget in the toplevel, it
//! is usually more convenient to use GTK2.Widget->grab_focus() instead of this
//! function.
//!
//!

GTK2.Window set_geometry_hints( GTK2.Widget widget, mapping geometry );
//! This function sets up hints about how a window can be resized by the user.
//! You can set a minimum and maximum size; allowed resized increments (e.g.
//! for xterm, you can only resize by the size of a character); aspect ratios;
//! and more.
//! geometry is a mapping with the following fields.  Any field which is
//! omitted is left as the default:
//! ([ "min_width": int,
//!    "min_height": int,
//!    "max_width": int,
//!    "max_height": int,
//!    "base_width": int,
//!    "base_height": int,
//!    "width_inc": int,
//!    "height_inc": int,
//!    "min_aspect": float,
//!    "max_aspect": float,
//!    "win_gravity": int, @[GDK_GRAVITY_CENTER], @[GDK_GRAVITY_EAST], @[GDK_GRAVITY_NORTH], @[GDK_GRAVITY_NORTH_EAST], @[GDK_GRAVITY_NORTH_WEST], @[GDK_GRAVITY_SOUTH], @[GDK_GRAVITY_SOUTH_EAST], @[GDK_GRAVITY_SOUTH_WEST], @[GDK_GRAVITY_STATIC] and @[GDK_GRAVITY_WEST]
//! ]);
//! min_width/min_height/max_width/max_height may be set to -1, and it will
//! substitute the size request of the window or geometry widget.  If the
//! minimum size hint is not provided, it will use its requisition as the
//! minimum size.  If the minimum size is provided and a geometry widget is
//! set, it will take the minimum size as the minimum size of the geometry
//! widget rather than the entire window.  The base size is treat similarly.
//! min_width: minimum width of the window (-1 to use requisition)
//! min_height: minimum height of window (-1 to use requisition)
//! max_width: maximum width of window (-1 to use requisition)
//! max_height: maximum height of window (-1 to use requisition)
//! base_width: allow window widths are base_width+width_inc*N (-1 allowed)
//! base_height: allowed window widths are base_height+height_inc*N (-1 allowed)
//! width_inc: width resize increment
//! height_inc: height resize increment
//! min_aspect: minimum width/height ratio
//! max_aspect: maximum width/height ratio
//! win_gravity: window gravity
//!
//!

GTK2.Window set_gravity( int gravity );
//! Window gravity defines the meaning of coordinates passed to move().
//! 
//! The default window gravity is GDK2.GRAVITY_NORTH_WEST, which will typically
//! "do what you mean."
//!
//!

GTK2.Window set_icon( GTK2.GdkPixbuf icon );
//! Sets up the icon representing this window.  This icon is used when the
//! window is minimized (also know as iconified).  Some window managers or
//! desktop environments may also place it in the window frame, or display it
//! in other contexts.
//! 
//! The icon should be provided in whatever size it was naturally drawn; that
//! is, don't scale the image before passing it.  Scaling is postponed until
//! the last minute, when the desired final size is known, to allow best
//! quality.
//! 
//! If you have your icon hand-drawn in multiple sizes, use set_icon_list().
//! Then the best size will be used.
//!
//!

GTK2.Window set_mnemonic_modifier( int modifier );
//! Sets the mnemonic modifer for this window.
//!
//!

GTK2.Window set_modal( int setting );
//! Sets a window modal or non-modal.  Modal windows prevent interaction with
//! other windows in the same application.  To keep modal dialogs on top of
//! main application windows, use set_transient_for() to make the dialog
//! transient for the parent; most window managers will then disallow lowering
//! the dialog below the parent.
//!
//!

GTK2.Window set_position( int pos );
//! Sets a position contraint for this window.  If the old or new constraint
//! is GTK2.WIN_POS_CENTER_ALWAYS, this will also cause the window to be
//! repositioned to satisfy the new constraint.
//!
//!

GTK2.Window set_resizable( int setting );
//! Sets whether the user can resize a window.  Windows are user resizable by
//! default.
//!
//!

GTK2.Window set_title( string title );
//! Set the window title. The default title is the value sent to
//! setup_gtk, or if none is sent, Pike GTK.
//!
//!

GTK2.Window set_transient_for( GTK2.Window parent );
//! Dialog window should be set transient for the main application window they
//! were spawned from.  This allows window managers to e.g. keep the dialog on
//! top of the main window, or center the dialog over the main window.
//! W(Dialog) and other convenience objects in GTK+ will sometimes call
//! set_transient_for on your behalf.
//!
//!

GTK2.Window set_type_hint( int hint );
//! Set type of window.  Values are @[GDK_WINDOW_TYPE_HINT_DIALOG], @[GDK_WINDOW_TYPE_HINT_MENU], @[GDK_WINDOW_TYPE_HINT_NORMAL] and @[GDK_WINDOW_TYPE_HINT_TOOLBAR]
//!
//!

GTK2.Window set_wmclass( string name, string class );
//! Set the window manager application name and class.
//!
//!

GTK2.Window stick( );
//! Makes this window sticky.  Same caveats as iconify().
//!
//!

GTK2.Window unmaximize( );
//! Opposite of maximize().
//!
//!

GTK2.Window unstick( );
//! Opposite of stick().
//!
//!
