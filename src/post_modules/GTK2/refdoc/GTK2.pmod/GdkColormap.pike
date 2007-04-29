//!

int alloc_color( GTK2.GdkColor color, int writeable, int best_match );
//! Alocates a single color from a colormap.
//!
//!

int alloc_colors( array colors, int writeable, int best_match );
//! Allocates colors from a colormap.
//!
//!

static GDK2.Colormap create( GTK2.GdkVisual visual, int|void allocate, int|void system );
//! Creates a new colormap.
//!
//!

GDK2.Colormap free_colors( array colors );
//! Free colors.
//!
//!

GTK2.GdkScreen get_screen( );
//! Returns the screen.
//!
//!

GTK2.GdkVisual get_visual( );
//! Returns the visual.
//!
//!

GTK2.GdkColor query_color( int pixel );
//! Locates the RGB color corresponding to the given hardware pixel.  pixel
//! must be a valid pixel in the colormap; it's a programmer error to call
//! this function with a pixel which is not in the colormap.  Hardware pixels
//! are normally obtained from alloc_colors(), or from a GDK2.Image.
//!
//!
