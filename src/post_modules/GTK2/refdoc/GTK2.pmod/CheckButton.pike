//! Check buttons inherent many properties and functions from the the
//! toggle buttons, but look a little different. Rather than
//! being buttons with text inside them, they are small squares with
//! the text to the right of them. These are often used for toggling
//! options on and off in applications.
//!@expr{ GTK2.CheckButton( "title" )@}
//!@xml{<image>../images/gtk2_checkbutton.png</image>@}
//!
//! Style properties:
//! int indicator-size
//! int indicator-spacing
//!
//!

inherit GTK2.ToggleButton;

static GTK2.CheckButton create( string|mapping label_or_props );
//! The argument, if specified, is the label of the item.
//! If no label is specified, use object->add() to add some
//! other widget (such as an pixmap or image widget)
//!
//!
