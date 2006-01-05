//! A horizontal scrollbar.
//! General documentation: See W(Scrollbar)
//!@expr{ GTK2.HScrollbar(GTK2.Adjustment())->set_size_request(300,15)@}
//!@xml{<image>../images/gtk2_hscrollbar.png</image>@}
//!
//!
//!
//!

inherit GTK2.Scrollbar;

static GTK2.HScrollbar create( GTK2.Adjustment adjustment_or_props );
//! Used to create a new hscrollbar widget.
//!
//!
