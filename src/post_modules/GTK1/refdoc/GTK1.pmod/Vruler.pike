//! Ruler widgets are used to indicate the location of the mouse
//! pointer in a given window. A window can have a vertical ruler
//! spanning across the width and a horizontal ruler spanning down the
//! height. A small triangular indicator on the ruler shows the exact
//! location of the pointer relative to the ruler.
//!
//!@expr{ GTK1.Vruler()->set_metric(GTK1.PIXELS)->set_range(0.0,100.0,50.0,100.0)->draw_ticks()->draw_pos()->set_usize(30,50)@}
//!@xml{<image>../images/gtk1_vruler.png</image>@}
//!
//!@expr{ GTK1.Vruler()->set_metric(GTK1.CENTIMETERS)->set_range(0.0,100.0,50.0,100.0)->draw_ticks()->draw_pos()->set_usize(30,50)@}
//!@xml{<image>../images/gtk1_vruler_2.png</image>@}
//!
//!@expr{ GTK1.Vruler()->set_usize(30,50)@}
//!@xml{<image>../images/gtk1_vruler_3.png</image>@}
//!
//!
//!

inherit GTK1.Ruler;

protected GTK1.Vruler create( );
//! Used to create a new vruler widget.
//!
//!
