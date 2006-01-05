//! Properties:
//! GTK2.RadioToolButton group
//!
//!

inherit GTK2.ToggleToolButton;

static GTK2.RadioToolButton create( GTK2.RadioToolButton groupmember );
//! Create a GTK2.RadioToolButton.
//! Use without a parameter for a new group.
//! Use with another GTK2.RadioToolButton to add another
//!   button to the same group as a previous button.
//!
//!

array get_group( );
//! Get the group this button belongs to.
//!
//!
