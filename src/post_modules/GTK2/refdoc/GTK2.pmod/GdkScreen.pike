//! Properties:
//! font-options
//! float resolution
//!
//!
//!  Signals:
//! @b{composite_changed@}
//!
//! @b{size_changed@}
//!

inherit G.Object;

static GDK2.Screen create( );
//! Gets the default screen.
//!
//!

GTK2.GdkColormap get_default_colormap( );
//! Gets the default colormap.
//!
//!

GTK2.GdkDisplay get_display( );
//! Gets the display to which this screen belongs.
//!
//!

int get_height( );
//! Gets the height in pixels.
//!
//!

int get_height_mm( );
//! Returns the height in millimeters.
//!
//!

int get_monitor_at_point( int x, int y );
//! Returns the monitor number in which x,y is located.
//!
//!

int get_monitor_at_window( GTK2.GdkWindow window );
//! Returns the number of the monitor in which the largest area of the
//! bounding rectangle of window resides.
//!
//!

GTK2.GdkRectangle get_monitor_geometry( int num );
//! Retrieves the GDK2.Rectangle representing the size and position of the
//! individual monitor within the entire screen area.
//!
//!

int get_n_monitors( );
//! Returns the number of monitors which this screen consists of.
//!
//!

int get_number( );
//! Gets the index of this screen among the screens in the display to which
//! it belongs.
//!
//!

GTK2.GdkColormap get_rgb_colormap( );
//! Gets the preferred colormap for rendering image data.
//!
//!

GTK2.GdkVisual get_rgb_visual( );
//! Get a "preferred visual" chosen by GdkRGB for rendering image data.
//!
//!

GTK2.GdkColormap get_rgba_colormap( );
//! Gets a colormap to use for creating windows or pixmaps with an alpha
//! channel.
//!
//!

GTK2.GdkVisual get_rgba_visual( );
//! Gets a visual to use for creating windows or pixmaps with an alpha
//! channel.
//!
//!

GTK2.GdkWindow get_root_window( );
//! Gets the root window.
//!
//!

GTK2.GdkColormap get_system_colormap( );
//! Gets the system default colormap.
//!
//!

GTK2.GdkVisual get_system_visual( );
//! Get the default system visual.
//!
//!

array get_toplevel_windows( );
//! Returns a list of all toplevel windows known to GDK on the screen.
//!
//!

int get_width( );
//! Gets the width of the screen in pixels.
//!
//!

int get_width_mm( );
//! Gets the width in millimeters.
//!
//!

array list_visuals( int|void def );
//! List the available visuals.  If def is true, return the available visuals
//! for the default screen.
//!
//!

string make_display_name( );
//! Determines the name to pass to GDK2.Display->open() to get a GDK2.Display
//! with this screen as the default screen.
//!
//!

GDK2.Screen set_default_colormap( GTK2.GdkColormap colormap );
//! Sets the default colormap.
//!
//!
