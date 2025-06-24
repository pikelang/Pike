// Automatically generated from "gtkwidget.pre".
// Do NOT edit.

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
//! int has-tooltip
//! int height-request
//! int is-focus
//! string name
//! int no-show-all
//! GTK2.Container parent
//! int receives-default
//! int sensitive
//! GTK2.Style style
//! string tooltip-markup
//! string tooltip-text
//! int visible
//! int width-request
//! 
//! Style properties:
//! float cursor-aspect-ratio
//! GDK2.Color cursor-color
//! int draw-border
//! string focus-line-pattern
//! int focus-line-width
//! int focus-padding
//! int interior-focus
//! GDK2.Color link-color
//! int scroll-arrow-hlength
//! int scroll-arrow-vlength
//! int separator-height
//! int separator-width
//! GDK2.Color visited-link-color
//! int wide-separators
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
//! @b{composited_changed@}
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
//! @b{drag_failed@}
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
//! @b{hide@}
//! Called when the widget is hidden
//!
//!
//! @b{hierarchy_changed@}
//!
//! @b{key_press_event@}
//! Called when a keyboard key is pressed
//!
//!
//! @b{key_release_event@}
//! Called when a keyboard key is released
//!
//!
//! @b{keynav_failed@}
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
//! @b{query_tooltip@}
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
//!

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

GTK2.Widget add_mnemonic_label( GTK2.Widget label );
//! Adds a widget to the list of mnemonic lables for this widget.  Note the
//! list of mnemonic labels for the widget is cleared when the widget is
//! destroyed, so the caller must make sure to update it's internal state at
//! this point as well, by using a connection to the destroy signal.
//!
//!

mapping allocation( );
//! Returns ([ "x": xoffset, "y":yoffset, "width":xsize, "height":ysize ])
//!
//!

int can_activate_accel( int signal_id );
//! Determines whether an accelerator that activates the signal signal_id can
//! currently be activated.
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

Pango.Context create_pango_context( );
//! Creates a new Pango.Context with the appropriate colormap, font
//! description, and base direction for drawing text for this widget.
//!
//!

Pango.Layout create_pango_layout( string text );
//! Creates a new Pango.Layout with the appropriate colormap, font
//! description, and base direction for drawing text.
//!
//!

GTK2.Widget drag_dest_set( int flags, array targets, int actions );
//! Set this widget up so it can accept drops. Parallel to @[drag_source_set]
//! - see there for arg info. flags allows default drop behaviour to turned on
//! and off; use GTK2.DEST_DEFAULT_ALL for normal behaviour.
//

GTK2.Widget drag_source_set( int buttons, array targets, int actions );
//! Set this widget up so a drag operation will start when the user clicks and
//! drags on this widget.
//! 
//! buttons is a bitwise 'or' of the mouse buttons that can initiate dragging,
//! eg GTK2.GDK_BUTTON1_MASK.
//! 
//! targets is an array of triples eg ({({"text/plain", GTK2.TARGET_SAME_APP, 0})})
//! where each triple gives an identifying MIME type, a flag specifying whether the
//! drag should be allowed to go to other applications, and an ID which will be
//! passed on to signal handlers. NOTE: For internal reasons, you are currently
//! permitted a maximum of ten targets.
//! 
//! actions is the set of valid actions that can be performed; use constants
//! GTK2.GDK_ACTION_COPY|GTK2.GDK_ACTION_MOVE etc.
//!
//!

GTK2.Widget error_bell( );
//! Notifies the user about an input-related error on this widget. If the 
//! "gtk-error-bell" setting is TRUE, it calls gdk_window_beep(), otherwise it 
//! does nothing
//!
//!

GTK2.Widget freeze_child_notify( );
//! Stops emission of "child-notify" signals.  The signals are queued until
//! thaw_child_notify() is called on the widget.
//!
//!

GTK2.Action get_action( );
//! Returns the GTK2.Action that this widget is a proxy for.
//!
//!

GTK2.Clipboard get_clipboard( GDK2.Atom selection );
//! Returns the clipboard object for the given selection.
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

GDK2.Display get_display( );
//! Get the GDK2.Display for the toplevel window associated with this widget.
//! This function can only be called after the widget has been added to a
//! widget hierarchy with a GTK2.Window at the top.
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

int get_no_show_all( );
//! Returns the current value of the "no-show-all" property.
//!
//!

Pango.Context get_pango_context( );
//! Gets a Pango.Context with the appropriate colormap, font description, and
//! base direction for this widget.
//!
//!

GTK2.Widget get_parent( );
//! Returns the parent container.
//!
//!

GDK2.Window get_parent_window( );
//! Get the parent window.
//!
//!

mapping get_pointer( );
//! Obtains the location of the mouse pointer in widget coordinates.
//!
//!

GDK2.Window get_root_window( );
//! Get the root window.
//!
//!

GDK2.Screen get_screen( );
//! Get the GDK2.Screen from the toplevel window associated with this widget.
//!
//!

GTK2.Settings get_settings( );
//! Gets the settings object holding the settings (global property settings,
//! RC file information, etc) used for this widget.
//!
//!

mapping get_size_request( );
//! Gets the size request that was explicityly set for the widget using
//! set_size_request().  A value of -1 for width or height indices that that
//! dimension has not been set explicityly and the natural requisition of
//! the widget will be used instead.  To get the size a widget will actually
//! use, call size_request() instead.
//!
//!

GDK2.Pixmap get_snapshot( GDK2.Rectangle clip_rect );
//! Create a GDK2.Pixmap of the contents of the widget and its children
//!
//! Works even if the widget is obscured.  The depth and visual of the
//! resulting pixmap is dependent on the widget being snapshot and likely
//! differs from those of a target widget displaying the pixmap.  The function
//! GDK2.Pixbuf->get_from_drawable() can be used to convert the pixmap to a
//! visual independent representation.
//!
//! The snapshot are used by this function is the widget's allocation plus
//! any extra space occupied by additional windows belonging to this widget
//! (such as the arrows of a spin button).  Thus, the resulting snapshot
//! pixmap is possibly larger than the allocation.
//!
//! If clip_rect is non-null, the resulting pixmap is shrunken to match the
//! specified clip_rect. The (x,y) coordinates of clip_rect are interpreted
//! widget relative.  If width or height of clip_rect are 0 or negative,
//! the width or height of the resulting pixmap will be shurnken by the
//! respective amount.  For instance, a clip_rect (+5,+5,-10,-10) will chop
//! off 5 pixels at each side of the snapshot pixmap. If non-null, clip_rect
//! will contain the exact widget-relative snapshot coordinates upon return.
//! A clip_rect of (-1,-1,0,0) can be used to preserve the auto-grown snapshot
//! area and use clip_rect as a pure output parameter.
//!
//! The return pixmap can be 0, if the resulting clip_area was empty.
//!
//!

GTK2.Style get_style( );
//! Returns the style.
//!
//!

string get_tooltip_markup( );
//! Gets the contents of the tooltip
//!
//!

string get_tooltip_text( );
//! Gets the contents of the tooltip
//!
//!

GTK2.Window get_tooltip_window( );
//! Returns the GtkWindow of the current tooltip
//!
//!

GTK2.Widget get_toplevel( );
//! This function returns the topmost widget in the container hierarchy this
//! widget is a part of.  If this widget has no parent widgets, it will be
//! returned as the topmost widget.
//!
//!

GDK2.Window get_window( );

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

int has_screen( );
//! Checks whether there is a GDK2.Screen associated with this widget.
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

GTK2.Widget input_shape_combine_mask( GDK2.Bitmap shape_mask, int offset_x, int offset_y );
//! Sets an input shape for this widget's GDK2.Window.
//!
//!

int is_ancestor( GTK2.Widget ancestor );
//! Determines whether this widget is somewhere inside ancestor.
//!
//!

int is_composited( );
//! Whether this widget can rely on having its alpha channel drawn correctly.
//!
//!

int is_focus( );
//! Determines if the widget is the focus widget within its toplevel.
//! (This does not mean that the HAS_FOCUS flag is necessarily set; HAS_FOCUS
//! will only be set if the toplevel widget additionally has the input focus.
//!
//!

int keynav_failed( int direction );
//! This function should be called whenever keyboard navigation within a 
//! single widget hits a boundary. The function emits the "keynav-changed" 
//! signal on the widget and its return value should be interpreted in a way 
//! similar to the return value of widget->child_focus():
//! 
//! When TRUE is returned, stay in the widget, the failed keyboard navigation 
//! is Ok and/or there is nowhere we can/should move the focus to.
//! 
//! When FALSE is returned, the caller should continue with keyboard 
//! navigation outside the widget, e.g. by calling widget->child_focus() 
//! on the widget's toplevel.
//! 
//! The default ::keynav-failed handler returns TRUE for GTK_DIR.TAB_FORWARD 
//! and GTK_DIR.TAB_BACKWARD. For the other values of GtkDirectionType, it 
//! looks at the "gtk-keynav-cursor-only"" setting and returns FALSE if the 
//! setting is TRUE. This way the entire user interface becomes 
//! cursor-navigatable on input devices such as mobile phones which only have 
//! cursor keys but no tab key.
//! 
//! Whenever the default handler returns TRUE, it also calls 
//! widget->error_bell() to notify the user of the failed keyboard navigation. 
//!
//!

array list_mnemonic_lables( );
//! Returns a list of the widgets, normally labels, for which this widget is
//! the target of a mnemonic.
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

GTK2.Widget modify_base( int state, GDK2.Color color );
//! Sets the base color of the widget in a particular state.
//! See modify_fg().
//!
//!

GTK2.Widget modify_bg( int state, GDK2.Color color );
//! Sets the background color of the widget in a particular state.
//! See modify_fg().
//!
//!

GTK2.Widget modify_cursor( GDK2.Color primary, GDK2.Color secondary );
//! Sets the cursor color to use in a widget.
//!
//!

GTK2.Widget modify_fg( int state, GDK2.Color color );
//! Sets the foreground color of the widget in a particular state.
//! state is one of @[STATE_ACTIVE], @[STATE_INSENSITIVE], @[STATE_NORMAL], @[STATE_PRELIGHT] and @[STATE_SELECTED].
//! color can be omitted to undo the effect of a previous call.
//!
//!

GTK2.Widget modify_font( Pango.FontDescription font );
//! Sets the font.
//!
//!

GTK2.Widget modify_style( GTK2.RcStyle style );
//! Modifies style values.
//!
//!

GTK2.Widget modify_text( int state, GDK2.Color color );
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

GTK2.Widget queue_resize_no_redraw( );
//! This function works like queue_resize(), except that the widget is not
//! invalidated.
//!
//!

int remove_accelerator( GTK2.AccelGroup accel_group, int accel_key, int accel_mods );
//! Removes an accelerator.
//!
//!

GTK2.Widget remove_mnemonic_label( GTK2.Widget label );
//! Removes a widget from the list of mnemonic labels for this widget.
//!
//!

GDK2.Pixbuf render_icon( string stock_id, int size, string detail );
//! A convenience function that uses the theme engine and rc file settings
//! to look up stock_id and render it to a pixbuf.  stock_id should be a
//! stock icon ID such as GTK2.STOCK_OPEN or GTK2.STOCK_OK.  size should be a
//! size such as GTK2.ICON_SIZE_MENU.  detail should be a string that identifies
//! the widget or code doing the rendering, so that theme engines can
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

GTK2.Widget set_no_show_all( int no_show_all );
//! Sets the "no-show-all" property, which determines whether calls to
//! show_all() and hide_all() will affect this widget.
//!
//!

GTK2.Widget set_parent( GTK2.Widget parent );
//! Sets the container as the parent of this widget.
//!
//!

GTK2.Widget set_parent_window( GDK2.Window parent );
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

GTK2.Widget set_tooltip_markup( string markup );
//! Sets markup as the contents of the tooltip
//!
//!

GTK2.Widget set_tooltip_text( string text );
//! Sets text as the contents of the tooltip.
//!
//!

GTK2.Widget set_tooltip_window( GTK2.Window window );
//! Replaces the default, usually yellow, window used for displaying tooltips.
//!
//!

GTK2.Widget shape_combine_mask( GDK2.Bitmap shape_mask, int offset_x, int offset_y );
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

mapping size_request( );
//! Get the size allocated to a widget.
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

GTK2.Widget trigger_tooltip_query( );
//! Triggers a tooltip query
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
