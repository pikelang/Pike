//! Gnome.DockBand is a widget implementing a "dock band", i.e. a
//! horizontal or vertical stripe containing dockable widgets.
//! 
//! The application programmer does not normally need to use
//! Gnome.DockBand directly; they are mostly used by the W(GnomeDock)
//! widget to implement its functionality. For an explanation of the
//! way dock bands are used within a dock, check out the documentation
//! for the W(GnomeDock) widget.
//! 
//!
//!

inherit GTK.Container;

int append( Gnome.DockItem child, int offset );
//! Add child to the band with the specified offset as the last element.
//!
//!

static Gnome.DockBand create( );
//! Create a new Gnome.DockBand widget.
//!
//!

int get_child_offset( GTK.Widget child );
//! Retrieve the offset of the child
//!
//!

Gnome.DockItem get_item_by_name( string name );
//! Retrieve a named item from the band.
//!
//!

int get_num_children( );
//! Retrieve the number of children
//!
//!

int get_orientation( );
//! Retrieve the orientation
//!
//!

int insert( Gnome.DockItem child, int offset, int position );
//! Add child to the band at the specified position, with the specified
//! offset from the previous item (or from the beginning of the band,
//! if this is the first item).
//!
//!

Gnome.DockBand layout_add( Gnome.DockLayout layout, int placement, int band_num );
//!

int prepend( Gnome.DockItem child, int offset );
//! Add child to the band with the specified offset as the first element.
//!
//!

Gnome.DockBand set_child_offset( GTK.Widget child, int offset );
//! Set the offset for the specified child of the band.
//!
//!

Gnome.DockBand set_orientation( int orientation );
//! Set the orientation.
//!
//!
