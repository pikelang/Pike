//! The basic widget, inherited (directly or indirectly) by all widgets.  Thus,
//! all functions and signals defined in this widget work on all widgets.
//! 
//! One of the most important functions in this class is 'show', it lets GTK2
//! know that we are done setting the attributes of the widget, and it is ready
//! to be displayed.  You may also use hide to make it disappear again.  The
//! order in which you show the widgets is not important, but I suggest showing
//! the toplevel window last so the whole window pops up at once rather than
//! seeing the individual widgets come up on the screen as they're formed.  The
//! children of a widget (a window is a widget too) will not be displayed until
//! the window itself is shown using the show() function.
//! 
//! Properties:
//! int app-paintable
//! int can-default
//! int can-focus
//! int composite-child
//! int events (GDK2.EventMask)
//! int extension-events (@[GDK_EXTENSION_EVENTS_ALL], @[GDK_EXTENSION_EVENTS_CURSOR] and @[GDK_EXTENSION_EVENTS_NONE])
//! int has-default
//! int has-focus
//! int height-request
//! int is-focus
//! string name
//! int no-show-all
//! GTK2.Container parent
//! int receives-default
//! int sensitive
//! GTK2.Style style
//! int visible
//! int width-request
//! 
//! Style properties:
//! float cursor-aspect-ratio
//! GDK2.Color cursor-color
//! string focus-line-pattern
//! int focus-line-width
//! int focus-padding
//! int interior-focus
//! GDK2.Color secondary-cursor-color
//!
//!
//!  Signals:
//! @b{accel_closures_changed@}
//!
//! @b{button_press_event@}
//! Called when a mouse button is pressed
//!
//!
//! @b{button_release_event@}
//! Called when a mouse button is released
//!
//!
//! @b{can_activate_accel@}
//!
//! @b{child_notify@}
//!
//! @b{client_event@}
//! An event sent by another client application
//!
//!
//! @b{configure_event@}
//! The size, position or stacking order of the widget has changed
//!
//!
//! @b{delete_event@}
//! Called when the user has requested that the widget should be closed
//!
//!
//! @b{destroy_event@}
//! Called when the widget is destroyed
//!
//!
//! @b{direction_changed@}
//!
//! @b{drag_begin@}
//! Called when the drag is initiated, on the sending side
//!
//!
//! @b{drag_data_delete@}
//! Called when the data can be safely deleted (there is no need to use this
//! function in pigtk, it's all handled automatically)
//!
//!
//! @b{drag_data_get@}
//! Called on the sending side when the drop is initiated
//!
//!
//! @b{drag_data_received@}
//! Called on the receiving side when the drop is finished
//!
//!
//! @b{drag_drop@}
//! Called on the receiving side when the drop is initiated
//!
//!
//! @b{drag_end@}
//! Called when the drag is finished, on the sending side
//!
//!
//! @b{drag_leave@}
//! Called when the mouse leaves the widget while the user is dragging something
//!
//!
//! @b{drag_motion@}
//! Called on the receiving side when the cursor is moved over the widget while
//! dragging something
//!
//!
//! @b{enter_notify_event@}
//! Calls when the mouse enters the widget
//!
//!
//! @b{event@}
//! Called for all events
//!
//!
//! @b{event_after@}
//!
//! @b{expose_event@}
//! Called when the widget, or a part of the widget, gets an expose event
//!
//!
//! @b{focus@}
//!
//! @b{focus_in_event@}
//! The keyboard focus has entered the widget
//!
//!
//! @b{focus_out_event@}
//! The keyboard focus has left the widget
//!
//!
//! @b{grab_broken_event@}
//!
//! @b{grab_focus@}
//!
//! @b{grab_notify@}
//!
//! @b{heirarchy_changed@}
//!
//! @b{hide@}
//! Called when the widget is hidden
//!
//!
//! @b{key_press_event@}
//! Called when a keyboard key is pressed
//!
//!
//! @b{key_release_event@}
//! Called when a keyboard key is released
//!
//!
//! @b{leave_notify_event@}
//! Called when the mouse leaves the widget
//!
//!
//! @b{map@}
//! Called when the window associated with the widget is mapped
//!
//!
//! @b{map_event@}
//! Called just before the 'map' signal
//!
//!
//! @b{mnemonic_activate@}
//!
//! @b{motion_notify_event@}
//! Called when the mouse is moved inside the widget
//!
//!
//! @b{no_expose_event@}
//!
//! @b{parent_set@}
//! Called when the parent widget is changed
//!
//!
//! @b{popup_menu@}
//!
//! @b{property_notify_event@}
//! Called when a property of the GDK window associated with the widget is
//! changed
//!
//!
//! @b{proximity_in_event@}
//!
//! @b{proximity_out_event@}
//!
//! @b{realize@}
//! Called when the widget is realized.  Some methods cannot be used until the
//! widget has been realized; if you get assertion errors related to
//! 'w->window' or similar, this is probably the cause.
//!
//!
//! @b{screen_changed@}
//!
//! @b{scroll_event@}
//!
//! @b{selection_clear_event@}
//!
//! @b{selection_get@}
//!
//! @b{selection_notify_event@}
//!
//! @b{selection_received@}
//!
//! @b{selection_request_event@}
//!
//! @b{show@}
//! Called when the widget is shown
//!
//!
//! @b{show_help@}
//!
//! @b{size_allocate@}
//! Called when the widget gets the size it should be
//!
//!
//! @b{size_request@}
//! Called when the widget should calculate how big it wants to be
//!
//!
//! @b{state_changed@}
//!
//! @b{style_set@}
//! Called when the style is changed
//!
//!
//! @b{unmap@}
//! Called when the window associated with the widget is unmapped
//!
//!
//! @b{unmap_event@}
//! Called just before the 'unmap' signal
//!
//!
//! @b{unrealize@}
//! Called when the widget is unrealized;
//!
//!
//! @b{visibility_notify_event@}
//! The widget has been mapped, unmapped, hidden, or otherwise had it's
//! visibility modified
//!
//!
//! @b{window_state_event@}
//!

inherit GTK2.Object;

GTK2.Widget APP_PAINTABLE( );
//! Returns if the GTK2.APP_PAINTABLE flag has been set on this widget.
//!
//!

GTK2.Widget CAN_DEFAULT( );
//! Returns if this widget is allowed to receive the default action.
//!
//!

GTK2.Widget CAN_FOCUS( );
//! Returns if this widget is able to handle focus grabs.
//!
//!

GTK2.Widget COMPOSITE_CHILD( );
//! Returns if this widget is a composite child of its parent.
//!
//!

GTK2.Widget DOUBLE_BUFFERED( );
//! Returns if the GTK2.DOUBLE_BUFFERED flag has been set on this widget.
//!
//!

GTK2.Widget DRAWABLE( );
//! Returns if this widget is mapped and visible.
//!
//!

GTK2.Widget FLAGS( );
//! Returns flags.
//!
//!

GTK2.Widget HAS_DEFAULT( );
//! Returns if this widget is currently receiving the default action.
//!
//!

GTK2.Widget HAS_FOCUS( );
//! Returns if this widget has grabbed the focus and no other widget has done
//! so more recently.
//!
//!

GTK2.Widget HAS_GRAB( );
//! Returns if this widget is in the grab_widgets stack, and will be the
//! preferred one for receiving events other than ones of cosmetic value.
//!
//!

GTK2.Widget IS_SENSITIVE( );
//! Returns if this widget is effectively sensitive.
//!
//!

GTK2.Widget MAPPED( );
//! Returns if this widget is mapped.
//!
//!

GTK2.Widget NO_WINDOW( );
//! Returns if this widget doesn't have its own GDK2.Window.
//!
//!

GTK2.Widget PARENT_SENSITIVE( );
//! Returns if the GTK2.PARENT_SENSITIVE flag is set.
//!
//!

GTK2.Widget RC_STYLE( );
//! Returns if the widget's style has been looked up through the rc mechanism.
//!
//!

GTK2.Widget REALIZED( );
//! Returns if this widget is realized.
//!
//!

GTK2.Widget RECEIVES_DEFAULT( );
//! Returns if this widget when focused will receive the default action even
//! if there is a different widget set as default.
//!
//!

GTK2.Widget SAVED_STATE( );
//! Get the saved state.
//!
//!

GTK2.Widget SENSITIVE( );
//! Returns if this widget is sensitive.
//!
//!

GTK2.Widget STATE( );
//! Get the state.
//!
//!

GTK2.Widget TOPLEVEL( );
//! Returns if this is a toplevel widget.
//!
//!

GTK2.Widget VISIBLE( );
//! Returns if this widget is visible.
//!
//!

int activate( );
//! For widgets that can be "activated" (buttons, menu items, etc.) this
//! functions activates them.  Activation is what happens when you press Enter
//! on a widget during key navigation.
//!
//!

GTK2.Widget add_accelerator( string accel_signal, GTK2.AccelGroup accel_group, int accel_key, int accel_mods, int accel_flags );
//! Installs an accelerator in accel_group that causes accel_signal to be
//! emitted if the accelerator is activated.  The signal must be of type
//! G_RUN_ACTION.
//!
//!

GTK2.Widget add_events( int events );
//! Adds the events in the bitfield events to the event mask.
//!
//!

GTK2.Widget child_notify( string prop );
//! Emits a "child-notify" signal for the child property prop.
//!
//!

string class_path( );
//! Same as path() but uses type rather than name.
//!
//!

GTK2.Widget freeze_child_notify( );
//! Stops emission of "child-notify" signals.  The signals are queued until
//! thaw_child_notify() is called on the widget.
//!
//!

string get_composite_name( );
//! Get the composite name.
//!
//!

int get_direction( );
//! Gets the reading direction.
//!
//!

int get_events( );
//! Returns the event mask for the widget.
//!
//!

int get_extension_events( );
//! Retrieves the extension events the widget will receive.
//!
//!

GTK2.RcStyle get_modifier_style( );
//! Returns the current modifier style.
//!
//!

string get_name( );
//! Retrieves the name.
//!
//!

GTK2.Widget get_parent( );
//! Returns the parent container.
//!
//!

GTK2.GdkWindow get_parent_window( );
//! Get the parent window.
//!
//!

GTK2.Style get_style( );
//! Returns the style.
//!
//!

GTK2.Widget get_toplevel( );
//! This function returns the topmost widget in the container hierarchy this
//! widget is a part of.  If this widget has no parent widgets, it will be
//! returned as the topmost widget.
//!
//!

GTK2.Widget grab_default( );
//! Causes this widget to become the default widget.
//!
//!

GTK2.Widget grab_focus( );
//! Causes this widget to have the keyboard focus.  This widget must be a
//! focusable widget, such as a GTK2.Entry; something like GTK2.Frame won't
//! work.
//!
//!

GTK2.Widget hide( );
//! Hide the widget.
//!
//!

GTK2.Widget hide_all( );
//! Hide this widget, and all child widgets.
//!
//!

int hide_on_delete( );
//! Utility function
//!
//!

int is_ancestor( GTK2.Widget ancestor );
//! Determines whether this widget is somewhere inside ancestor.
//!
//!

int is_focus( );
//! Determines if the widget is the focus widget within its toplevel.
//! (This does not mean that the HAS_FOCUS flag is necessarily set; HAS_FOCUS
//! will only be set if the toplevel widget additionally has the input focus.
//!
//!

GTK2.Widget map( );
//! Causes this widget to be mapped.
//!
//!

int mnemonic_activate( int group_cycling );
//! Not documented.
//!
//!

GTK2.Widget modify_base( int state, GTK2.GdkColor color );
//! Sets the base color of the widget in a particular state.
//! See modify_fg().
//!
//!

GTK2.Widget modify_bg( int state, GTK2.GdkColor color );
//! Sets the background color of the widget in a particular state.
//! See modify_fg().
//!
//!

GTK2.Widget modify_fg( int state, GTK2.GdkColor color );
//! Sets the foreground color of the widget in a particular state.
//! state is one of @[STATE_ACTIVE], @[STATE_INSENSITIVE], @[STATE_NORMAL], @[STATE_PRELIGHT] and @[STATE_SELECTED].
//! color can be omitted to undo the effect of a previous call.
//!
//!

GTK2.Widget modify_style( GTK2.RcStyle style );
//! Modifies style values.
//!
//!

GTK2.Widget modify_text( int state, GTK2.GdkColor color );
//! Sets the text color of the widget in a particular state.
//! See modify_fg().
//!
//!

string path( );
//! Obtains the full path.  The path is simply the name of a widget and all
//! its parents in the container hierarchy, separated by periods.
//!
//!

GTK2.Widget queue_draw( );
//! Equivalent to calling queue_draw_area() for the entire area of the widget.
//!
//!

GTK2.Widget queue_draw_area( int x, int y, int width, int height );
//! Invalidates the rectangular area defined by x,y,width,height by calling
//! GDK2.Window->invalidate_rect() on the widget's window and all its child
//! windows.  Once the main loop becomse idle (after the current batch of
//! events has been processed, roughly), the window will receive expose events
//! for the union of all regions that have been invalidated.
//!
//!

GTK2.Widget queue_resize( );
//! This function is only for use in widget implementations.  Flags a widget
//! to have its size renegotiated; should be called when a widget for some
//! reason has a new size request.  For example, when you change the text in a
//! W(Label), GTK2.Label queues a resize to ensure there's enough space for
//! the new text.
//!
//!

int remove_accelerator( GTK2.AccelGroup accel_group, int accel_key, int accel_mods );
//! Removes an accelerator.
//!
//!

GTK2.GdkPixbuf render_icon( string stock_id, int size, string detail );
//! A convenience function that uses the theme engine and rc file settings
//! to look up stock_id and render it to a pixbuf.  stock_id should be a
//! stock icon ID such as GTK2.STOCK_OPEN or GTK2.STOCK_OK.  size should be a
//! size such as GTK2.ICON_SIZE_MENU.  detail should be a string that identifies//! the widget or code doing the rendering, so that theme engines can
//! special-case rendering for that widget or code.
//! 
//! The pixels in the returned GDK2.Pixbuf are shared with the rest of the
//! application and should not be modified.
//!
//!

GTK2.Widget reparent( GTK2.Widget new_parent );
//! Moves a widget from one W(Container) to another.
//!
//!

GTK2.Widget reset_shapes( );
//! Recursively resets the shape on this widget and its descendants.
//!
//!

GTK2.Widget set_accel_path( string accel_path, GTK2.AccelGroup accel_group );
//! Sets up an accelerator in accel_group so whenever the key binding that is
//! defined for accel_path is pressed, this widget will be activated.
//!
//!

GTK2.Widget set_composite_name( string name );
//! Sets a widget's composite name.
//!
//!

GTK2.Widget set_direction( int dir );
//! Sets the reading direction.  This direction controls the primary direction
//! for widgets containing text, and also the direction in which the children
//! of the container are packed.  The ability to set the direction is present
//! in order so that correct localization into languages with right-to-left
//! reading directions can be done.  Generally, applications will let the
//! default reading direction present, except for containers where the
//! containers are arranged in an order that is explicitly visual rather than
//! logical (such as buttons for text justification).
//!
//!

GTK2.Widget set_events( int events );
//! Sets the event mask for a widget.  The event mask determines which events
//! a widget will receive.  Keep in mind that different widgets have different
//! default event masks, and by changing the event mask you may disrupt a
//! widget's functionality, so be careful.  This function must be called while
//! a widget is unrealized.  Consider add_events() for widgets that are already
//! realized, or if you want to preserve the existing event mask.  This
//! function can't be used with GTK2.NO_WINDOW widgets; to get events on those
//! widgets, place them inside a GTK2.EventBox and receive events on the event
//! box.
//!
//!

GTK2.Widget set_extension_events( int mode );
//! Sets the extension events mask to mode.  One of @[GDK_EXTENSION_EVENTS_ALL], @[GDK_EXTENSION_EVENTS_CURSOR] and @[GDK_EXTENSION_EVENTS_NONE]
//!
//!

GTK2.Widget set_flags( int flags );
//! Set certain flags, such as GTK2.CAN_DEFAULT.
//!
//!

GTK2.Widget set_name( string name );
//! Widgets can be named, which allows you to refer to them from a gtkrc file.
//! You can apply a style to widgets with a particular name in the gtkrc file.
//!
//!

GTK2.Widget set_parent( GTK2.Widget parent );
//! Sets the container as the parent of this widget.
//!
//!

GTK2.Widget set_parent_window( GTK2.GdkWindow parent );
//! Sets a non default parent window for this widget.
//!
//!

GTK2.Widget set_scroll_adjustments( GTK2.Adjustment hadj, GTK2.Adjustment vadj );
//! For widgets that support scrolling, sets the scroll adjustments.  For
//! widgets that don't support scrolling, does nothing.  Widgets that don't
//! support scrolling can be scrolled by placing them in a W(Viewport), which
//! does support scrolling.
//!
//!

GTK2.Widget set_sensitive( int sensitive );
//! Sets the sensitivity.  A widget is sensitive is the user can interact with
//! it.  Insensitive widgets are "grayed out" and the user can't interact with
//! them.  Insensitive widgets are known as "inactive", "disabled", or
//! "ghosted" in some other toolkits.
//!
//!

GTK2.Widget set_size_request( int width, int height );
//! Sets the minimum size of a widget; that is, the widget's size request will
//! be width by height.  You can use this function to force a widget to be
//! either larger or smaller than it normally would be.
//! 
//! In most cases, set_default_size() is a better choice for toplevel windows
//! than this function; setting the default size will still allow users to
//! shrink the window.  Setting the size request will force them to leave the
//! window at least as large as the size request.  When dealing with window
//! sizes, set_geometry_hints() can be a useful function as well.
//! 
//! Note the inherent danger of setting any fixed size - themes, translations
//! into other languages, different fonts, and user action can all change the
//! appropriate size for a given widget.  So, it's basically impossible to
//! hardcode a size that will always be correct.
//! 
//! The size request of a widget is the smallest size a widget can accept while
//! still functioning well and drawing itself correctly.  However in some
//! strange cases a widget may be allocated less than its requested size, and
//! in many cases a widget may be allocated more space than it request.
//! 
//! If the size request in a given direction is -1 (unset), then the "natural"
//! size request of the widget will be used instead.
//! 
//! Widgets can't actually be allocated a size less than 1x1, but you can pass
//! 0,0 to this function to mean "as small as possible".
//!
//!

GTK2.Widget set_style( GTK2.Style style );
//! Sets the style.
//!
//!

GTK2.Widget shape_combine_mask( GTK2.GdkBitmap shape_mask, int offset_x, int offset_y );
//! Sets a shape for a widget's GDK2.Window.  This allows for transparent
//! window, etc.
//!
//!

GTK2.Widget show( );
//! Show the widget.  Most (almost all) widgets must be shown to be
//! visible on the screen.
//!
//!

GTK2.Widget show_all( );
//! Show this widget, and all child widgets.
//!
//!

GTK2.Widget show_now( );
//! Shows a widget.  If the widget is an unmapped toplevel widget (i.e. a
//! GTK2.Window that has not yet been shown), enter the main loop and wait for
//! the window to actually be mapped.  Be careful; because the main loop is
//! running, anything can happen during this function.
//!
//!

mixed style_get_property( string name );
//! Gets the value of the style property called name.
//!
//!

GTK2.Widget thaw_child_notify( );
//! Reverts the effect of a previous call to freeze_child_notify().
//!
//!

GTK2.Widget unmap( );
//! Causes this widget to be unmapped.
//!
//!

GTK2.Widget unset_flags( int flags );
//! Unset flags.
//!
//!
