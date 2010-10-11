//!

static GDK.Region create( );
//! Create a new (empty) region
//!
//! NOIMG
//!
//!

GDK.Region destroy( );
//!

int equal( GDK.Region victim );
//! Return true if the region used as an argument is equal to the
//! current region. Also available as a==b when a is a region.
//!
//!

GDK.Region intersect( GDK.Region victim );
//! Computes the intersection of the given region and the region. Also
//! available as region &amp; region
//!
//!

GDK.Region offset( int dx, int dy );
//! Offset(move) the region by dx,dy pixels.
//!
//!

int point_in( int x, int y );
//! Returns true if the given point resides in the given region
//!
//!

int rect_in( GDK.Rectangle r );
//! Returns true if the given rectangle resides inside the given region
//!
//!

GDK.Region shrink( int dx, int dy );
//! reduces the size of a region by a
//! specified amount.  Positive values shrink the size of the
//! region, and negative values expand the region.
//!
//!

GDK.Region subtract( GDK.Region victim );
//! Computes the difference of the given region and the region.
//! Also available as region - region
//!
//!

GDK.Region union( GDK.Region victim );
//! Computes the union of the given rectangle or region and the region. 
//! Also available as region | rectangle, region | region, region +
//! region and region + rectangle.
//!
//!

GDK.Region xor( GDK.Region victim );
//! Computes the exlusive or of the given region and the region.
//! Also available as region ^ region
//!
//!
