//! General documentation: See W(Scrollbar)
//!@expr{ GTK.Vscrollbar(GTK.Adjustment())->set_usize(15,60)@}
//!@xml{<image>../images/gtk_vscrollbar.png</image>@}
//!
//!
//!
//!

inherit GTK.Scrollbar;

static GTK.Vscrollbar create( GTK.Adjustment pos );
//! Used to create a new vscrollbar widget.
//! The adjustment argument can either be an existing W(Adjustment), or
//! 0, in which case one will be created for you. Specifying 0 might
//! actually be useful in this case, if you wish to pass the newly
//! automatically created adjustment to the constructor function of
//! some other widget which will configure it for you, such as a text
//! widget.
//!
//!
