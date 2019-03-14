//! The color selection dialog widget is, not surprisingly, a color selection
//! widget in a dialog window. Use the subwidget functions below to access the
//! different subwidgets directly.
//! 
//!@expr{ GTK1.ColorSelectionDialog("Select color")@}
//!@xml{<image>../images/gtk1_colorselectiondialog.png</image>@}
//!
//! 
//!
//!
//!  Signals:
//! @b{color_changed@}
//! Called when the color is changed
//!
//!

inherit GTK1.Window;

protected GTK1.ColorSelectionDialog create( string title );
//! Create a new color selection dialog with the specified title.
//!
//!

GTK1.Button get_cancel_button( );
//! Return the cancel button widget.
//!
//!

GTK1.ColorSelection get_colorsel( );
//! Return the color selection widget
//!
//!

GTK1.Button get_help_button( );
//! Return the help button
//!
//!

GTK1.Button get_ok_button( );
//! Return the ok button
//!
//!
