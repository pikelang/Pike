//! Ruler widgets are used to indicate the location of the mouse
//! pointer in a given window. A window can have a vertical ruler
//! spanning across the width and a horizontal ruler spanning down
//! the height. A small triangular indicator on the ruler shows the
//! exact location of the pointer relative to the ruler.
//!
//!
inherit Widget;

Ruler draw_pos( )
//! draw the position
//!
//!

Ruler draw_ticks( )
//! draw the ticks
//!
//!

float get_lower( )
//! The currently defined lower extent of the ruler.
//!
//!

float get_max_size( )
//! The currently defined max_size of the ruler.
//!
//!

float get_position( )
//! The currently defined initial position of the pointer indicator
//! within the ruler.
//!
//!

float get_upper( )
//! The currently defined upper extent of the ruler.
//!
//!

Ruler set_metric( int unit )
//! Either GTK.Pixels, GTK.Centimers or GTK.Inches.
//! The default measure is GTK.Pixels.
//!
//!

Ruler set_range( float lower, float upper, float position, float max_size )
//! The lower and upper arguments define the extent of the ruler, and
//! max_size is the largest possible number that will be displayed.
//! Position defines the initial position of the pointer indicator
//! within the ruler.
//!
//!
