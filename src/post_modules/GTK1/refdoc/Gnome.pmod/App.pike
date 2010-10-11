//! Toplevel GNOME applications would normally use one Gnome.App widget
//! as their toplevel window. You can create as many Gnome.App widgets
//! as you want, for example, some people use one GnomeApp per document
//! their application loads.
//! 
//! Once you have created one instance of this widget, you would add
//! your main application view information to this window by using
//! set_contents() routine.
//! 
//! The GnomeApp has support for including a menubar, one or more
//! toolbars and a statusbar for your application. It also takes care
//! of intalling the accelerators for you when used in conjuction with
//! the gnome-app-helper routines. The toolbars are inserted into
//! Gnome.Dock widgets.
//! 
//! The gnome-app-helper module provides various helper routines to
//! simplify the configuration of your menus and toolbars, but you can
//! create those yourself and use the set_menus(), add_toolbar(),
//! set_toolbar(), add_dock_item() and add_docked().
//!
//!

inherit GTK.Window;

Gnome.App add_dock_item( Gnome.DockItem item, int placement, int band_num, int band_position, int|void offset );
//! Create a new Gnome.DockItem widget containing widget, and add it
//! to app's dock with the specified layout information. Notice that,
//! if automatic layout configuration is enabled, the layout is
//! overridden by the saved configuration, if any.
//! 
//!
//! item : Item to be added to app's dock
//! placement :  Placement for the new dock item, one of Gnome.DockTop,  Gnome.DockRight,  Gnome.DockBottom,  Gnome.DockLeft and  Gnome.DockFloating 
//! band_num : Number of the band where the dock item should be placed 
//! band_position : Position of the new dock item in band band_num
//! offset : Offset from the previous dock item in the band; if there is no previous item, offset from the beginning of the band.
//! 
//!
//!

Gnome.App add_docked( GTK.Widget widget, string name, int behavior, int placement, int band_num, int band_position, int|void offset );
//! Create a new Gnome.DockItem widget containing widget, and add it
//! to app's dock with the specified layout information. Notice that,
//! if automatic layout configuration is enabled, the layout is
//! overridden by the saved configuration, if any.
//! 
//!
//! widget : Widget to be added to app's dock
//! name : Name for the dock item that will contain toolbar
//! behavior : Behavior for the new dock item. One of @[GNOME_DOCK_ITEM_BEH_EXCLUSIVE], @[GNOME_DOCK_ITEM_BEH_LOCKED], @[GNOME_DOCK_ITEM_BEH_NEVER_FLOATING], @[GNOME_DOCK_ITEM_BEH_NEVER_HORIZONTAL], @[GNOME_DOCK_ITEM_BEH_NEVER_VERTICAL] and @[GNOME_DOCK_ITEM_BEH_NORMAL]
//! placement :  Placement for the new dock item, one of Gnome.DockTop,  Gnome.DockRight,  Gnome.DockBottom,  Gnome.DockLeft and  Gnome.DockFloating 
//! band_num : Number of the band where the dock item should be placed 
//! band_position : Position of the new dock item in band band_num 
//! offset : Offset from the previous dock item in the band; if there is no previous item, offset from the beginning of the band.
//! 
//!
//!

Gnome.App add_toolbar( GTK.Toolbar toolbar, string name, int behavior, int placement, int band_num, int band_position, int|void offset );
//! Create a new Gnome.DockItem widget containing toolbar, and add it
//! to app's dock with the specified layout information. Notice that,
//! if automatic layout configuration is enabled, the layout is
//! overridden by the saved configuration, if any.
//! 
//!
//! toolbar : Toolbar to be added to app's dock
//! name : Name for the dock item that will contain toolbar
//! behavior : Behavior for the new dock item. One or more of @[GNOME_DOCK_ITEM_BEH_EXCLUSIVE], @[GNOME_DOCK_ITEM_BEH_LOCKED], @[GNOME_DOCK_ITEM_BEH_NEVER_FLOATING], @[GNOME_DOCK_ITEM_BEH_NEVER_HORIZONTAL], @[GNOME_DOCK_ITEM_BEH_NEVER_VERTICAL] and @[GNOME_DOCK_ITEM_BEH_NORMAL]
//! placement :  Placement for the new dock item, one of Gnome.DockTop,  Gnome.DockRight,  Gnome.DockBottom,  Gnome.DockLeft and  Gnome.DockFloating 
//! band_num : Number of the band where the dock item should be placed
//! band_position : Position of the new dock item in band band_num
//! offset : Offset from the previous dock item in the band; if there is no previous item, offset from the beginning of the band.
//! 
//!
//!

static Gnome.App create( string appname, string|void title );
//! Create a new (empty) application window. You must specify the
//! application's name (used internally as an identifier). title can be
//! left as 0, in which case the window's title will not be set.
//!
//!

Gnome.App enable_layout_config( int enable );
//! Specify whether the the dock's layout configuration should be
//! automatically saved via gnome-config whenever it changes, or not.
//!
//!

Gnome.Dialog error( string error );
//! An important fatal error; if it appears in the statusbar, it might
//! gdk_beep() and require acknowledgement.
//!
//!

Gnome.App flash( string flash );
//! Flash the message in the statusbar for a few moments; if no
//! statusbar, do nothing. For trivial little status messages,
//! e.g. "Auto saving..."
//!
//!

Gnome.Dock get_dock( );
//! retrieved the Gnome.Dock widget contained in the App
//!
//!

Gnome.DockItem get_dock_item_by_name( string name );
//!

Gnome.Dialog message( string message );
//! A simple message, in an OK dialog or the status bar. Requires
//! confirmation from the user before it goes away.
//! Returns 0 or a dialog widget. If 0, the message is displayed in the
//! status bar.
//!
//!

Gnome.App ok_cancel( string question, function callback, mixed cb_arg2 );
//! Ask a ok or cancel question
//! and call the callback when it's answered.
//!
//!

Gnome.App ok_cancel_modal( string question, function callback, mixed cb_arg2 );
//! Ask a ok or cancel question, block the application while it is asked,
//! and call the callback when it's answered.
//!
//!

Gnome.AppProgressKey progress_manual( string prompt );
//!

Gnome.AppProgressKey progress_timeout( string prompt, int interval, function cb, mixed cb_arg1 );
//!

Gnome.App question( string question, function reply_callback, mixed cb_arg2 );
//! Ask a yes or no question, and call the callback when it's answered.
//!
//!

Gnome.App question_modal( string question, function callback, mixed cb_arg2 );
//! Ask a yes or no question, block the application while it is asked,
//! and call the callback when it's answered.
//!
//!

Gnome.App request_password( string question, function callback, mixed cb_arg2 );
//! As request string, but do not show the string
//!
//!

Gnome.App request_string( string question, function callback, mixed cb_arg2 );
//! Request a string, and call the callback when it's answered.
//!
//!

Gnome.App set_contents( GTK.Widget contents );
//! Sets the status bar of the application window.
//!
//!

Gnome.App set_menus( GTK.MenuBar menu_bar );
//! Sets the menu bar of the application window.
//!
//!

Gnome.App set_statusbar( GTK.Widget statusbar );
//! Sets the status bar of the application window.
//!
//!

Gnome.App set_toolbar( GTK.Toolbar toolbar );
//! Sets the main toolbar of the application window.
//!
//!

Gnome.Dialog warning( string warning );
//! A not-so-important error, but still marked better than a flash
//!
//!
