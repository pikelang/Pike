//! Ruler widgets are used to indicate the location of the mouse
//! pointer in a given window. A window can have a vertical ruler
//! spanning across the width and a horizontal ruler spanning down the
//! height. A small triangular indicator on the ruler shows the exact
//! location of the pointer relative to the ruler.
//!
//!@code{ GTK.Hruler()->set_metric(GTK.PIXELS)->set_range(0.0,100.0,50.0,100.0)->draw_ticks()->draw_pos()->set_usize(300,30)@}
//!@xml{<image>../images/gtk_hruler.png</image>@}
//!
//!@code{ GTK.Hruler()->set_metric(GTK.CENTIMETERS)->set_range(0.0,100.0,50.0,100.0)->draw_ticks()->draw_pos()->set_usize(300,30)@}
//!@xml{<image>../images/gtk_hruler_2.png</image>@}
//!
//!@code{ GTK.Hruler()->set_usize(300,30)@}
//!@xml{<image>../images/gtk_hruler_3.png</image>@}
//!
//!
//!

inherit GTK.Ruler;

static GTK.Hruler create( );
//! Used to create a new hruler widget.
//!
//!
