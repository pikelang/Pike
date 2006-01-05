//! Gnome2.Canvas is an engine for structured graphics that offers a rich
//! imaging model, high performance rendering, and a powerful, high level API.
//! It offers a choice of two rendering back-ends, one based on Xlib for
//! extremely fast display, and another based on Libart, a sophisticated,
//! antialiased, alpha-compositing engine.  This widget can be used for
//! flexible display of graphics and for creating interactive user interface
//! elements.
//! 
//! A Gnome2.Canvas widget contains one or more Gnome2.CanvasItem objects.
//! Items consist of graphing elements like lines, ellipses, polygons, images,
//! text, and curves.  These items are organized using Gnome2.CanvasGroup
//! objects, which are themselves derived from Gnome2.CanvasItem.  Since a
//! group is an item it can be contained within other groups, forming a tree
//! of canvas items.  Certain operations, like translating and scaling, can be
//! performed on all items in a group.
//! See @xml{<url  href="http://developer.gnome.org/doc/API/2.0/libgnomecanvas/GnomeCanvas.html">
//! http://developer.gnome.org/doc/API/2.0/libgnomecanvas/GnomeCanvas.html
//! </url>@} for more information.
//! Properties:
//! int aa
//!
//!
//!  Signals:
//! @b{draw_background@}
//!
//! @b{render_background@}
//!

inherit GTK2.Layout;

array c2w( int cx, int cy );
//! Converts canvas pixel coordinates to world coordinates.
//!
//!

static Gnome2.Canvas create( int|void anti_alias );
//! Create a new Gnome2.Canvas widget.  Set anti_alias to true to create
//! a canvas in antialias mode.
//!
//!

int get_center_scroll_region( );
//! Returns whether the canvas is set to center the scrolling region in the
//! window if the former is smaller than the canvas' allocation.
//!
//!

GTK2.GdkColor get_color( string|void spec );
//! Allocates a color based on the specified X color specification.  An
//! omitted or empty string is considered transparent.
//!
//!

int get_color_pixel( int rgba );
//! Allocates a color from the RGBA value passed into this function.  The
//! alpha opacity value is discarded, since normal X colors do not support it.
//!
//!

int get_dither( );
//! Returns the type of dithering used to render an antialiased canvas.
//!
//!

GTK2.Gnome2CanvasItem get_item_at( float x, float y );
//! Looks for the item that is under the specified position, which must be
//! specified in world coordinates.
//!
//!

array get_scroll_offsets( );
//! Queries the scrolling offsets of a canvas.  The values are returned in
//! canvas pixel units.
//!
//!

mapping get_scroll_region( );
//! Queries the scrolling region of a canvas.
//!
//!

GTK2.Gnome2CanvasGroup root( );
//! Queries the root group.
//!
//!

Gnome2.Canvas scroll_to( int cx, int cy );
//! Makes a canvas scroll to the specified offsets, given in canvas pixel
//! units.  The canvas will adjust the view so that it is not outside the
//! scrolling region.  This function is typically not used, as it is better to
//! hook scrollbars to the canvas layout's scrolling adjustments.
//!
//!

Gnome2.Canvas set_center_scroll_region( int setting );
//! When the scrolling region of the canvas is smaller than the canvas window,
//! e.g. the allocation of the canvas, it can be either centered on the
//! window or simply made to be on the upper-left corner on the window.
//!
//!

Gnome2.Canvas set_dither( int dither );
//! Controls the dithered rendering for antialiased canvases.  The value of
//! dither should be GDK2.RgbDitherNone, GDK2.RgbDitherNormal, or
//! GDK2.RgbDitherMax.  The default canvas setting is GDK2.RgbDitherNormal.
//!
//!

Gnome2.Canvas set_pixels_per_unit( float n );
//! Sets the zooming factor of a canvas by specifying the number of pixels that
//! correspond to one canvas unit.
//! 
//! The anchor point for zooming, i.e. the point that stays fixed and all
//! others zoom inwards or outwards from it, depends on whether the canvas is
//! set to center the scrolling region or not.  You can contorl this using the
//! set_center_scroll_region() function.  If the canvas is set to center the
//! scroll region, then the center of the canvas window is used as the anchor
//! point for zooming.  Otherwise, the upper-left corner of the canvas window
//! is used as the anchor point.
//!
//!

Gnome2.Canvas set_scroll_region( float x1, float y1, float x2, float y2 );
//! Sets the scrolling region of a canvas to the specified rectangle.  The
//! canvas will then be able to scroll only within this region.  The view of
//! the canvas is adjusted as appropriate to display as much of the new region
//! as possible.
//!
//!

array w2c( float wx, float wy );
//! Converts world coordinates into canvas pixel coordinates.
//!
//!

array w2c_affine( );
//! Gets the affine transform that converts from world coordinates to canvas
//! pixel coordinates.
//!
//!

array w2c_d( float wx, float wy );
//! Converts world coordinates into canvas pixel coordinates.  This version
//! returns coordinates in floating point coordinates, for greater precision.
//!
//!

array window_to_world( float winx, float winy );
//! Converts window-relative coordinates into world coordinates.  You can use
//! this when you need to convert mouse coordinates into world coordinates, for
//! example.
//!
//!

array world_to_window( float worldx, float worldy );
//! Converts world coordinates into window-relative coordinates.
//!
//!
