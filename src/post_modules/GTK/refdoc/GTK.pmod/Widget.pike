//! The basic widget, inherited (directly or indirectly) by all
//! widgets.  Thus, all functions and signals defined in this widget
//! works on all widgets.
//! 
//! One of the most importat functions in this class is 'show',
//! it lets GTK know that we are done setting the attributes of the
//! widget, and it is ready to be displayed. You may also use hide to
//! make it disappear again. The order in which you show the widgets is
//! not important, but I suggest showing the toplevel window last so
//! the whole window pops up at once rather than seeing the individual
//! widgets come up on the screen as they're formed. The children of a
//! widget (a window is a widget too) will not be displayed until the
//! window itself is shown using the show() function.
//!
//!
//!  Signals:
//! @b{add_accelerator@}
//! Called when an accelarator (keyboard shortcut) is added to the widget
//!
//!
//! @b{button_press_event@}
//! Called when a mouse button is pressed
//!
//!
//! @b{button_release_event@}
//! Called when a mouse button is released
//!
//!
//! @b{client_event@}
//! An event sent by another client application
//!
//!
//! @b{configure_event@}
//! The size, position or stacking order of the widget has changed
//!
//!
//! @b{debug_msg@}
//!
//! @b{delete_event@}
//! Called when the user has requested that the widget should be closed
//!
//!
//! @b{destroy_event@}
//! Called when the widget is destroyed
//!
//!
//! @b{drag_begin@}
//! Called when the drag is initiated, on the sending side
//!
//!
//! @b{drag_data_delete@}
//! Called when the data can be safely deleted (there is no need to use this function in pigtk, it's all handled automatically)
//!
//!
//! @b{drag_data_get@}
//! Called on the sending side when the drop is initiated
//!
//!
//! @b{drag_data_received@}
//! Called on the receiving side when the drop is finished.
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
//! Called on the receiving side when the cursor is moved over the widget while dragging something
//!
//!
//! @b{draw@}
//! Called when the widget should draw itself.
//!
//!
//! @b{draw_default@}
//! Called when the widget should draw itself, and indicate that it's the default widget
//!
//!
//! @b{draw_focus@}
//! Called when the widget should draw itself, and a focus indicator around itself (or otherwise indicate that is has the keyboard focus)
//!
//!
//! @b{enter_notify_event@}
//! Called when the mouse enters the widget
//!
//!
//! @b{event@}
//! Called for all events
//!
//!
//! @b{expose_event@}
//! Called when the widget, or a part of the widget gets and expose event
//!
//!
//! @b{focus_in_event@}
//! The keyboard focus has entered the widget
//!
//!
//! @b{focus_out_event@}
//! The keyboard focus has left the widget
//!
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
//! @b{motion_notify_event@}
//! Called when the mouse is moved inside the widget
//!
//!
//! @b{no_expose_event@}
//!
//! @b{other_event@}
//!
//! @b{parent_set@}
//! Called when the parent widget is changed
//!
//!
//! @b{property_notify_event@}
//! Called when a property of the GDK window associated with the widget is changed
//!
//!
//! @b{proximity_in_event@}
//!
//! @b{proximity_out_event@}
//!
//! @b{realize@}
//! Called when the widget is realized. Some methods cannot be used until the widget has been realized, if you get assertion errors related to 'w->window' or similar, this is probably the cause.
//!
//!
//! @b{remove_accelerator@}
//! Called when an accelerator (keyboard shortcut) is removed from the widget
//!
//!
//! @b{selection_clear_event@}
//! NYI
//!
//!
//! @b{selection_notify_event@}
//! NYI
//!
//!
//! @b{selection_received@}
//! NYI
//!
//!
//! @b{selection_request_event@}
//! NYI
//!
//!
//! @b{show@}
//! Called when the widget is shown
//!
//!
//! @b{size_allocate@}
//! Called when the widget get's the size it should be
//!
//!
//! @b{size_request@}
//! Called when the widget should calculate how big it want's to be
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
//! Called when the widget is unrealized.
//!
//!
//! @b{visibility_notify_event@}
//! The widget has been mapped, unmapped, hidden, or otherwise had it's visibility modified
//!
//!

inherit GTK.Object;

GTK.Widget activate( );
//! Activate the widget. This either activates the widget, if possible
//! (as an example, buttons can be activated), or activates the default
//! widget of it's parent (or it's parent, or it's parents parent
//! etc.)
//!
//!

GTK.Widget add_accelerator( string signal, GTK.AccelGroup group, int key, int modifiers, int flags );
//! Add an accelerator (keyboard shortcut).
//! 
//! Flag is one of @[ACCEL_LOCKED], @[ACCEL_SIGNAL_VISIBLE] and @[ACCEL_VISIBLE]
//! 
//! The signal is the signal to invoke when the accelerator key is pressed.
//! 
//! The modifiers is a bitwise or of one or more of GDK.ShiftMask,
//! GDK.LockMask, GDK.ControlMask, @[GDK_MOD1_MASK], @[GDK_MOD2_MASK], @[GDK_MOD3_MASK], @[GDK_MOD4_MASK] and @[GDK_MOD5_MASK].
//! 
//! The group is the accelerator group in which the accelerator should be added.
//! 
//! The key is the unicode code for the key to bind.
//!
//!

GTK.Widget add_events( int events );
//! Events is one or more of:
//! GDK.ExposureMask, GDK.PointerMotionMask,
//! GDK.PointerMotion_HINTMask, GDK.ButtonMotionMask,
//! GDK.Button1MotionMask, GDK.Button2MotionMask,
//! GDK.Button3MotionMask, GDK.ButtonPressMask, GDK.ButtonReleaseMask,
//! GDK.KeyPressMask, GDK.KeyReleaseMask, GDK.EnterNotifyMask,
//! GDK.LeaveNotifyMask, GDK.FocusChangeMask, GDK.StructureMask,
//! GDK.PropertyChangeMask, GDK.VisibilityNotifyMask,
//! GDK.ProximityInMask, GDK.ProximityOutMask and GDK.AllEventsMask
//!
//!

mapping allocation( );
//! Returns ([ "x":xoffset, "y":yoffset, "width":xsize, "height":ysize ])
//!
//!

string class_path( );
//! Returns the class 'pathname' of this widget. Useful for rc-files and such.
//!
//!

GTK.Widget copy_area( GDK.GC gc, int xdest, int ydest, GTK.Widget source, int xsource, int ysource, int width, int height );
//! Copies the rectangle defined by xsource,ysource and width,height
//! from the source widget, and places the results at xdest,ydest in
//! the widget in which this function is called.
//! NOTE: The widget must be realized before this function can be used
//!
//!

GTK.Widget drag_dest_set( int flags, array targets, int actions );
//!     Register a drop site, and possibly add default behaviors.
//!   arguments:
//!     flags:     Which types of default drag behavior to use (one of @[DEST_DEFAULT_ALL], @[DEST_DEFAULT_DROP], @[DEST_DEFAULT_HIGHLIGHT] and @[DEST_DEFAULT_MOTION])
//!     targets:   Table of targets that can be accepted
//!            ({ ({ content_type(string), flags(int(try 0)), id(int) }), ...})
//!       The id will be received in the signal handlers.
//!     actions:   one of @[GDK_ACTION_ASK], @[GDK_ACTION_COPY], @[GDK_ACTION_DEFAULT], @[GDK_ACTION_LINK], @[GDK_ACTION_MOVE] and @[GDK_ACTION_PRIVATE]
//!   results:
//!
//!

GTK.Widget drag_dest_unset( );
//! Removes the drop support from this widget (see drag_dest_set)
//!
//!

GTK.Widget drag_get_data( GDK.DragContext ctx, int time );
//! Get the data from a context and an integer timestamp (from an
//! event), requesting it from the dropping client. This function
//! should probably never be used directly.
//!
//!Use the selection argument passed to the drag_data_received signal
//! instead.
//!
//!

GTK.Widget drag_highlight( );
//! Highlight the widget. Not normaly used directly.
//!
//!

GTK.Widget drag_source_set( int flags, array targets, int actions );
//!     Register a drop site, and possibly add default behaviors.
//!   arguments:
//!     buttons:     Which mouse buttons can be used to start the drag
//!     targets:   Table of targets that can be accepted
//!            ({ ({ content_type(string), flags(int(try 0)), id(int) }), ...})
//!       The id will be received in the signal handlers.
//!     actions:   one of @[GDK_ACTION_ASK], @[GDK_ACTION_COPY], @[GDK_ACTION_DEFAULT], @[GDK_ACTION_LINK], @[GDK_ACTION_MOVE] and @[GDK_ACTION_PRIVATE]
//!   results:
//!
//!

GTK.Widget drag_source_set_icon( GDK.Pixmap pm, GDK.Bitmap mask );
//! Set the icon that will be used (by default) for drags
//! from this widget.
//!
//!

GTK.Widget drag_source_unset( );
//! Remove the drag support from this widget. See drag_source_set.
//!
//!

GTK.Widget drag_unhighlight( );
//! Unhighlight the widget. Not normaly used directly.
//!
//!

GTK.Widget ensure_style( );
//! Ensure that the widget has a style associated with it.
//!
//!

string get_composite_name( );
//! Like set name, but it is inherited by the children of this widget.
//!
//!

int get_events( );
//! Return the current event mask (see set_events and add_events)
//!
//!

int get_extension_events( );
//! Returns one of @[GDK_EXTENSION_EVENTS_ALL], @[GDK_EXTENSION_EVENTS_CURSOR] and @[GDK_EXTENSION_EVENTS_NONE]
//!
//!

GDK.Window get_gdkwindow( );
//! Return the GDK.Window associated with this widget, if any.
//!
//!

string get_name( );
//! Returns the name set by set_name or the class name
//!
//!

GTK.Widget get_parent( );
//! Returns the parent of this widget, if any, if there is no parent
//! 0 is returned.
//!
//!

GTK.Style get_style( );
//! Return the style associated with this widget
//!
//!

GTK.Widget get_toplevel( );
//! return the toplevel widget this widget is inside (or this widget if
//! it is the toplevel widget)
//!
//!

GTK.Widget grab_default( );
//! Make this widget the default action for the parent widget
//!
//!

GTK.Widget grab_focus( );
//! Grab the focus.
//!
//!

int has_set_flags( int mask );
//! All possible flags are:
//! GTK.AppPaintable,
//! GTK.CanDefault,
//! GTK.CanFocus,
//! GTK.CompositeChild,
//! GTK.HasDefault,
//! GTK.HasFocus,
//! GTK.HasGrab,
//! GTK.Mapped,
//! GTK.NoReparent,
//! GTK.NoWindow,
//! GTK.ParentSensitive,
//! GTK.RcStyle,
//! GTK.Realized,
//! GTK.ReceivesDefault
//! GTK.Sensitive,
//! GTK.Toplevel and
//! GTK.Visible.
//!
//!

GTK.Widget hide( );
//! Hide this widget
//!
//!

GTK.Widget hide_all( );
//! Hide this widget and all it's children
//!
//!

int intersect( GDK.Rectangle area, GDK.Rectangle intersection );
//! Do an intersection with the area and place the result in
//! 'intersection'.
//!
//!

int is_ancestor( GTK.Widget of );
//! Returns true if the specified widget is an ancestor of this widget.
//!
//!

GTK.Widget lock_accelerators( );
//! Make it impossible to add new accelerators, or remove old ones
//!
//!

GTK.Widget map( );
//! Map the widget. Should normally not be called directly.
//!
//!

string path( );
//! Returns the 'pathname' of this widget. Useful for rc-files and such.
//!
//!

GTK.Widget popup( int xpos, int ypos );
//! Map the widget at the specified coordinates.
//!
//!

GTK.Widget queue_clear( );
//! Force a clear (and subsequent redraw) of the widget
//!
//!

GTK.Widget queue_clear_area( int x, int y, int width, int height );
//! Force a clear (and subsequent redraw) of part of the widget
//!
//!

GTK.Widget queue_draw( );
//! Force a redraw of the widget
//!
//!

GTK.Widget queue_draw_area( int x, int y, int width, int height );
//! Force a redraw of part of the widget
//!
//!

GTK.Widget queue_resize( );
//! Force a allocation recalculation, followed by a redraw
//!
//!

GTK.Widget realize( );
//! Realize this widget, and it's children, if nessesary
//!
//!

GTK.Widget remove_accelerator( GTK.AccelGroup group, int key, int modifiers );
//! Remove an accelerator (keyboard shortcut).
//! 
//! The modifiers is a bitwise or of one or more of GDK.ShiftMask,
//! GDK.LockMask, GDK.ControlMask, @[GDK_MOD1_MASK], @[GDK_MOD2_MASK], @[GDK_MOD3_MASK], @[GDK_MOD4_MASK] and @[GDK_MOD5_MASK].
//! 
//! The group is the accelerator group in which the accelerator should be added.
//! 
//! The key is the unicode code for the key to bind.
//!
//!

GTK.Widget reparent( GTK.Widget to );
//! Change the parent of the widget.
//!
//!

GTK.Widget reset_rc_styles( );
//! Reset all styles to their default value, recursively
//!
//!

GTK.Widget restore_default_style( );
//! Reset all styles to their default value
//!
//!

GTK.Widget selection_add_target( GDK.Atom|void selection, GDK.Atom|void target, int|void info );
//! Supplying the selection is a bit more complicated than requesting
//! it. You must register handlers that will be called when your
//! selection is requested. For each selection/target pair you will
//! handle, you make a call to this function.
//!
//! selection, and target identify the requests this handler will
//! manage. When a request for a selection is received, the
//! "selection_get" signal will be called. info can be used as an
//! enumerator to identify the specific target within the callback
//! function.
//!
//! Selection defaults to PRIMARY, and target defaults to STRING.
//!
//!

GTK.Widget selection_owner_set( GDK.Atom|void selection, int|void time );
//! When prompted by the user, you claim ownership of the selection by
//! calling this function.
//!
//! If another application claims ownership of the selection, you will
//! receive a "selection_clear_event".
//!
//! The selection defaults to the PRIMARY selection, and the time to
//! GDK.CurrentTime.
//!
//!

int selecton_convert( GDK.Atom|void selection, GDK.Atom|void target, int|void time );
//! Retrieving the selection is an asynchronous process. To start the
//! process, you call this function.
//!
//! This converts the selection into the form specified by target. If
//! at all possible, the time field should be the time from the event
//! that triggered the selection. This helps make sure that events
//! occur in the order that the user requested them. However, if it is
//! not available (for instance, if the conversion was triggered by a
//! "clicked" signal), then you can omit it altogether. This will
//! cause it to be set to GDK.CurrentTime.
//!
//! When the selection owner responds to the request, a
//! "selection_received" signal is sent to your application. The
//! handler for this signal receives a pointer to a GTK.SelectionData
//! object.
//!
//! The special target TARGETS generates a list of all valid targets.
//!
//! Selection defaults to PRIMARY, and target defaults to STRING.
//!
//!

GTK.Widget set_app_paintable( int paintablep );
//! Make it possible to draw directly in the widget using the low-level
//! drawing functions.
//!
//!

GTK.Widget set_background( GDK.Color background );
//! Set the background color or image.
//! The argument is either a GDK.Pixmap or a GDK.Color object.
//! NOTE: The widget must be realized before this function can be used
//!
//!

GTK.Widget set_bitmap_cursor( GDK.Bitmap source, GDK.Bitmap mask, GDK.Color fg, GDK.Color bg, int xhot, int yhot );
//! xhot and yhot are the locations of the x and y hotspot relative to the
//! upper left corner of the cursor image.
//! Both the bitmaps and the colors must be specified.
//!
//!

GTK.Widget set_composite_name( string name );
//! Like set name, but it is inherited by the children of this widget.
//!
//!

GTK.Widget set_cursor( int|void cursor_type, GDK.Color fg, GDK.Color bg );
//! Change the widget cursor.
//! If no arguments are passed, restore the default cursor.
//! Both fg and bg must be specified if either one is.
//! <table border="0" cellpadding="3" cellspacing="0">
//! CURS(GDK.Arrow)
//! CURS(GDK.BasedArrowDown)
//! CURS(GDK.BasedArrowUp)
//! CURS(GDK.Boat)
//! CURS(GDK.Bogosity)
//! CURS(GDK.BottomLeftCorner)
//! CURS(GDK.BottomRightCorner)
//! CURS(GDK.BottomSide)
//! CURS(GDK.BottomTee)
//! CURS(GDK.BoxSpiral)
//! CURS(GDK.CenterPtr)
//! CURS(GDK.Circle)
//! CURS(GDK.Clock)
//! CURS(GDK.CoffeeMug)
//! CURS(GDK.Cross)
//! CURS(GDK.CrossReverse)
//! CURS(GDK.Crosshair)
//! CURS(GDK.DiamondCross)
//! CURS(GDK.Dot)
//! CURS(GDK.Dotbox)
//! CURS(GDK.DoubleArrow)
//! CURS(GDK.DraftLarge)
//! CURS(GDK.DraftSmall)
//! CURS(GDK.DrapedBox)
//! CURS(GDK.Exchange)
//! CURS(GDK.Fleur)
//! CURS(GDK.Gobbler)
//! CURS(GDK.Gumby)
//! CURS(GDK.Hand1)
//! CURS(GDK.Hand2)
//! CURS(GDK.Heart)
//! CURS(GDK.Icon)
//! CURS(GDK.IronCross)
//! CURS(GDK.LeftPtr)
//! CURS(GDK.LeftSide)
//! CURS(GDK.LeftTee)
//! CURS(GDK.Leftbutton)
//! CURS(GDK.LlAngle)
//! CURS(GDK.LrAngle)
//! CURS(GDK.Man)
//! CURS(GDK.Middlebutton)
//! CURS(GDK.Mouse)
//! CURS(GDK.Pencil)
//! CURS(GDK.Pirate)
//! CURS(GDK.Plus)
//! CURS(GDK.QuestionArrow)
//! CURS(GDK.RightPtr)
//! CURS(GDK.RightSide)
//! CURS(GDK.RightTee)
//! CURS(GDK.Rightbutton)
//! CURS(GDK.RtlLogo)
//! CURS(GDK.Sailboat)
//! CURS(GDK.SbDownArrow)
//! CURS(GDK.SbHDoubleArrow)
//! CURS(GDK.SbLeftArrow)
//! CURS(GDK.SbRightArrow)
//! CURS(GDK.SbUpArrow)
//! CURS(GDK.SbVDoubleArrow)
//! CURS(GDK.Shuttle)
//! CURS(GDK.Sizing)
//! CURS(GDK.Spider)
//! CURS(GDK.Spraycan)
//! CURS(GDK.Star)
//! CURS(GDK.Target)
//! CURS(GDK.Tcross)
//! CURS(GDK.TopLeftArrow)
//! CURS(GDK.TopLeftCorner)
//! CURS(GDK.TopRightCorner)
//! CURS(GDK.TopSide)
//! CURS(GDK.TopTee)
//! CURS(GDK.Trek)
//! CURS(GDK.UlAngle)
//! CURS(GDK.Umbrella)
//! CURS(GDK.UrAngle)
//! CURS(GDK.Watch)
//! CURS(GDK.Xterm)
//! </table>
//!
//!

GTK.Widget set_events( int events );
//! Events is one or more of:
//! GDK.ExposureMask, GDK.PointerMotionMask,
//! GDK.PointerMotion_HINTMask, GDK.ButtonMotionMask,
//! GDK.Button1MotionMask, GDK.Button2MotionMask,
//! GDK.Button3MotionMask, GDK.ButtonPressMask, GDK.ButtonReleaseMask,
//! GDK.KeyPressMask, GDK.KeyReleaseMask, GDK.EnterNotifyMask,
//! GDK.LeaveNotifyMask, GDK.FocusChangeMask, GDK.StructureMask,
//! GDK.PropertyChangeMask, GDK.VisibilityNotifyMask,
//! GDK.ProximityInMask, GDK.ProximityOutMask and GDK.AllEventsMask
//!
//!

GTK.Widget set_extension_events( int events );
//! Events is one of @[GDK_EXTENSION_EVENTS_ALL], @[GDK_EXTENSION_EVENTS_CURSOR] and @[GDK_EXTENSION_EVENTS_NONE]
//!
//!

GTK.Widget set_flags( int flags );
//! The flags that it makes sense to set are:
//! GTK.CanFocus and GTK.CanDefault
//!
//!

GTK.Widget set_name( string name );
//! Set the name of the widget. The name is used when the rc-file is
//! parsed, you can also parse your own resources by calling
//! GTK.parse_rc() with a resource string. Example: 
//! TODO
//!
//!

GTK.Widget set_rc_style( );
//! Set the style from the .rc files
//!
//!

int set_scroll_adjustments( GTK.Adjustment hadjustment, GTK.Adjustment vadjustment );
//! Set the scrolling (panning) adjustment objects for this widget.
//! Returns 1 if it is possible to do so, and 0 otherwise.
//!
//!

GTK.Widget set_sensitive( int sensitivep );
//! True (1) or false (0). If true, the widget can receive events,
//! otherwise the user cannot interact with the widget. Most widgets
//! are drawn 'greyed' or more dim when they are unsensitive.
//!
//!

GTK.Widget set_state( int state );
//! One of @[STATE_ACTIVE], @[STATE_INSENSITIVE], @[STATE_NORMAL], @[STATE_PRELIGHT] and @[STATE_SELECTED].
//! This function should normaly not be used directly.
//!
//!

GTK.Widget set_style( GTK.Style cf );
//! Set the style to be associated with this widget
//!
//!

GTK.Widget set_uposition( int xpos, int ypos );
//! Set the absolute coordinates of the widget relative to it's parent.
//!
//!

GTK.Widget set_usize( int xsize, int ysize );
//! Set the absolute size of the widget. It might resize itself anyway,
//! but this size is used as is in most widgets. Beware of this
//! function, it might produce unexpected results. 0 for any size means
//! 'keep old size'. When setting sizes of wtoplevel windows, it is
//! preferable to use set_default_size
//!
//!

GTK.Widget shape_combine_mask( GDK.Bitmap shape, int xoffset, int yoffset );
//! Set the shape of the widget, or, rather, it's window, to that of
//! the supplied bitmap. Notice how the window behind the example
//! window can be seen because of the rather odd shape the example window has.
//!@expr{ GTK.Window( GTK.WINDOW_TOPLEVEL )->add(GTK.Label("A rather Oddly shaped\n" "Window\n" "Indeed\n" "Or what do you\nthink?\n" "This text\n" "should\n" "be long enough"))->shape_combine_mask( GDK.Bitmap(Image.image(100,100,255,255,255)->rotate(10,0,0,0) ), 20,20)@}
//!@xml{<image>../images/gtk_widget_shape_combine_mask.png</image>@}
//!
//! NOTE: The widget must be realized before this function can be used
//!
//!

GTK.Widget show( );
//! Show the widget. Most (almost all) widgets must be shown to be
//! visible on the screen.
//!
//!

GTK.Widget show_all( );
//! Show this widget and all it's children
//!
//!

GTK.Widget show_now( );
//! Show this widget and do not return until it is visible.
//!
//!

int text_width( string text );
//! Returns the width, in pixels, the string would have if it was
//! written with the default font in the style object assosiated with
//! the widget.
//!
//!

GTK.Widget unlock_accelerators( );
//! Make it possible to add new accelerators, and remove old ones,
//! again. Use this after lock_accelerators()
//!
//!

GTK.Widget unmap( );
//! Unmap the widget. Should normally not be called directly.
//!
//!

GTK.Widget unparent( );
//! Remove this widget from it's parent
//!
//!

GTK.Widget unrealize( );
//! Unrealize this widget, and it's children, if nessesary
//!
//!

GTK.Widget unset_flags( int flags );
//! The flags that it makes sense to unset are:
//! GTK.CanFocus and GTK.CanDefault
//!
//!

int xoffset( );
//! Returns the x position of the upper left corner relative to the
//! widgets window in pixels. For widgets that have their own window
//! this will most likely be 0.
//!
//!

int xsize( );
//! Returns the width of the widget in pixels.
//!
//!

int yoffset( );
//! Returns the y position of the upper left corner relative to the
//! widgets window in pixels. For widgets that have their own window
//! this will most likely be 0.
//!
//!

int ysize( );
//! Returns the height of the widget in pixels.
//!
//!
