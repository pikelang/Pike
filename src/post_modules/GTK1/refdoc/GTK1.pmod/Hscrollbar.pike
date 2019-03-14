//! A horizontal scrollbar.
//! General documentation: See W(Scrollbar)
//!@expr{ GTK1.Hscrollbar(GTK1.Adjustment())->set_usize(300,15)@}
//!@xml{<image>../images/gtk1_hscrollbar.png</image>@}
//!
//!
//!
//!

inherit GTK1.Scrollbar;

protected GTK1.Hscrollbar create( GTK1.Adjustment adjustment );
//! Used to create a new vscale widget.
//! The adjustment argument can either be an existing W(Adjustment), or
//! 0, in which case one will be created for you. Specifying 0 might
//! actually be useful in this case, if you wish to pass the newly
//! automatically created adjustment to the constructor function of
//! some other widget which will configure it for you, such as a text
//! widget.
//!
//!
