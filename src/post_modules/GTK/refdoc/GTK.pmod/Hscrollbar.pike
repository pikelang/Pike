//! A horizontal scrollbar.
//! General documentation: See W(Scrollbar)
//!@code{ GTK.Hscrollbar(GTK.Adjustment())->set_usize(300,15)@}
//!@xml{<image>../images/gtk_hscrollbar.png</image>@}
//!
//!
//!
//!

inherit GTK.Scrollbar;

static GTK.Hscrollbar create( GTK.Adjustment adjustment );
//! Used to create a new vscale widget.
//! The adjustment argument can either be an existing W(Adjustment), or
//! 0, in which case one will be created for you. Specifying 0 might
//! actually be useful in this case, if you wish to pass the newly
//! automatically created adjustment to the constructor function of
//! some other widget which will configure it for you, such as a text
//! widget.
//!
//!
