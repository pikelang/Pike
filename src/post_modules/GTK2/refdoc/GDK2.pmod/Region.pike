// Automatically generated from "gdkregion.pre".
// Do NOT edit.

//! Region information.
//!
//!


protected void create( );
//! Create a new (empty) region
//!
//! NOIMG
//!
//!

int equal( GDK2.Region victim );
//! Return true if the region used as an argument is equal to the
//! current region. Also available as a==b when a is a region.
//!
//!

GDK2.Region intersect( GDK2.Region victim );
//! Computes the intersection of the given region and the region. Also
//! available as region &amp; region
//!
//!

GDK2.Region offset( int dx, int dy );
//! Offset(move) the region by dx,dy pixels.
//!
//!

int point_in( int x, int y );
//! Returns true if the given point resides in the given region
//!
//!

int rect_in( GDK2.Rectangle r );
//! Returns true if the given rectangle resides inside the given region
//!
//!

GDK2.Region shrink( int dx, int dy );
//! reduces the size of a region by a
//! specified amount.  Positive values shrink the size of the
//! region, and negative values expand the region.
//!
//!

GDK2.Region subtract( GDK2.Region victim );
//! Computes the difference of the given region and the region.
//! Also available as region - region
//!
//!

GDK2.Region union( GDK2.Region victim );
//! Computes the union of the given rectangle or region and the region. 
//! Also available as region | rectangle, region | region, region +
//! region and region + rectangle.
//!
//!

GDK2.Region xor( GDK2.Region victim );
//! Computes the exlusive or of the given region and the region.
//! Also available as region ^ region
//!
//!
