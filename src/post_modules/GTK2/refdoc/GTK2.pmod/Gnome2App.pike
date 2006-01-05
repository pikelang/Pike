//! Toplevel GNOME applications would normally use one Gnome2.App widget
//! as their toplevel window. You can create as many Gnome2.App widgets
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
//! Gnome2.Dock widgets.
//! 
//! The gnome-app-helper module provides various helper routines to
//! simplify the configuration of your menus and toolbars, but you can
//! create those yourself and use the set_menus(), add_toolbar(),
//! set_toolbar(), add_dock_item() and add_docked().
//! Properties:
//! string app-id
//!
//!

inherit GTK2.Window;

Gnome2.App add_docked( GTK2.Widget widget, string name, int behavior, int placement, int band_num, int band_position, int|void offset );
//! Create a new Gnome2.DockItem widget containing widget, and add it
//! to app's dock with the specified layout information. Notice that,
//! if automatic layout configuration is enabled, the layout is
//! overridden by the saved configuration, if any.
//! 
//!
//! widget : Widget to be added to app's dock
//! name : Name for the dock item that will contain toolbar
//! behavior : Behavior for the new dock item. One of 
//! placement :  Placement for the new dock item, one of Gnome2.DockTop,  Gnome2.DockRight,  Gnome2.DockBottom,  Gnome2.DockLeft and  Gnome2.DockFloating 
//! band_num : Number of the band where the dock item should be placed 
//! band_position : Position of the new dock item in band band_num 
//! offset : Offset from the previous dock item in the band; if there is no previous item, offset from the beginning of the band.
//! 
//!
//!

Gnome2.App add_toolbar( GTK2.Toolbar toolbar, string name, int behavior, int placement, int band_num, int band_position, int|void offset );
//! Create a new Gnome2.DockItem widget containing toolbar, and add it
//! to app's dock with the specified layout information. Notice that,
//! if automatic layout configuration is enabled, the layout is
//! overridden by the saved configuration, if any.
//! 
//!
//! toolbar : Toolbar to be added to app's dock
//! name : Name for the dock item that will contain toolbar
//! behavior : Behavior for the new dock item. One or more of 
//! placement :  Placement for the new dock item, one of Gnome2.DockTop,  Gnome2.DockRight,  Gnome2.DockBottom,  Gnome2.DockLeft and  Gnome2.DockFloating 
//! band_num : Number of the band where the dock item should be placed
//! band_position : Position of the new dock item in band band_num
//! offset : Offset from the previous dock item in the band; if there is no previous item, offset from the beginning of the band.
//! 
//!
//!

static Gnome2.App create( string appname, string|void title );
//! Create a new (empty) application window. You must specify the
//! application's name (used internally as an identifier). title can be
//! left as 0, in which case the window's title will not be set.
//!
//!

Gnome2.App enable_layout_config( int enable );
//! Specify whether the the dock's layout configuration should be
//! automatically saved via gnome-config whenever it changes, or not.
//!
//!

Gnome2.App set_contents( GTK2.Widget contents );
//! Sets the status bar of the application window.
//!
//!

Gnome2.App set_menus( GTK2.MenuBar menu_bar );
//! Sets the menu bar of the application window.
//!
//!

Gnome2.App set_statusbar( GTK2.Widget statusbar );
//! Sets the status bar of the application window.
//!
//!

Gnome2.App set_statusbar_custom( GTK2.Widget container, GTK2.Widget statusbar );
//! Sets the status bar of the application window, but use container as its
//! container.
//!
//!

Gnome2.App set_toolbar( GTK2.Toolbar toolbar );
//! Sets the main toolbar of the application window.
//!
//!
