//! A GdkVisual describes a particular video hardware display format.
//! It includes information about the number of bits used for each
//! color, the way the bits are translated into an RGB value for
//! display, and the way the bits are stored in memory.
//!
//!

inherit G.Object;

static GDK2.Visual create( int|void best, int|void depth, int|void type );
//! @param best
//!   If best is false the systems default GDK screen is returned,
//!   otherwise the screen that best fulfills the given depth and
//!   type. If none is given, the one with most colors is selected.
//! @param depth
//!   The requested bit depth, or 0.
//! @param type
//!   The requested visual type.
//!   @int
//!     @value GDK_VISUAL_STATIC_GRAY
//!       Each pixel value indexes a grayscale value directly.
//!     @value GDK_VISUAL_GRAYSCALE
//!       Each pixel is an index into a color map that maps pixel
//!       values into grayscale values. The color map can be changed
//!       by an application.
//!     @value GDK_VISUAL_STATIC_COLOR
//!       Each pixel value is an index into a predefined, unmodifiable
//!       color map that maps pixel values into RGB values.
//!     @value GDK_VISUAL_PSEUDO_COLOR
//!       Each pixel is an index into a color map that maps pixel
//!       values into rgb values. The color map can be changed by an
//!       application.
//!     @value GDK_VISUAL_TRUE_COLOR
//!       Each pixel value directly contains red, green, and blue
//!       components. The red_mask, green_mask, and blue_mask fields
//!       of the GdkVisual structure describe how the components are
//!       assembled into a pixel value.
//!     @value GDK_VISUAL_DIRECT_COLOR
//!       Each pixel value contains red, green, and blue components as
//!       for @[GDK_VISUAL_TRUE_COLOR], but the components are mapped via
//!       a color table into the final output table instead of being
//!       converted directly.
//!   @endint
//


GTK2.GdkScreen get_screen( );
//! Gets the screen to which this visual belongs.
//!
//!
