//! The GnomeMessageBox widget creates dialog boxes (of type
//! GnomeDialog) that contain a severity level (indicated by an icon
//! and a title), a message to be displayed and a list of buttons that
//! will be in the dialog.
//! 
//! The programmer will use strings desired for each button. If the
//! strings are any of the GNOME_STOCK macros, then instead of creating
//! a button with the text, the button will be a GNOME stock button
//! with a stock icon.
//! 
//! The list of known types for message boxes are:
//! @[GNOME_MESSAGE_BOX_ERROR], @[GNOME_MESSAGE_BOX_GENERIC], @[GNOME_MESSAGE_BOX_INFO], @[GNOME_MESSAGE_BOX_QUESTION] and @[GNOME_MESSAGE_BOX_WARNING].
//!@expr{ Gnome.MessageBox( "This is a nice message", Gnome.MessageBoxInfo, Gnome.StockButtonOk,  Gnome.StockButtonCancel );@}
//!@xml{<image>../images/gnome_messagebox.png</image>@}
//!
//!@expr{ Gnome.MessageBox( "This is another not so nice message", Gnome.MessageBoxError, Gnome.StockButtonClose,  Gnome.StockButtonCancel );@}
//!@xml{<image>../images/gnome_messagebox_2.png</image>@}
//!
//! 
//!
//!

inherit Gnome.Dialog;

static Gnome.MessageBox create( string message, string messagebox_type, string... buttons );
//! Creates a dialog box of type message_box_type with message. A
//! number of buttons are inserted on it. You can use the GNOME stock
//! identifiers to create gnome stock buttons.
//!
//!
