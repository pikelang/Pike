//! The color selection dialog widget is, not surprisingly, a color selection
//! widget in a dialog window. Use the subwidget functions below to access the
//! different subwidgets directly.
//! 
//!@code{ GTK.ColorSelectionDialog("Select color")@}
//!@xml{<image>../images/gtk_colorselectiondialog.png</image>@}
//!
//! 
//!
//!
//!  Signals:
//! @b{color_changed@}
//! Called when the color is changed
//!
//!

inherit GTK.Window;

static GTK.ColorSelectionDialog create( string title );
//! Create a new color selection dialog with the specified title.
//!
//!

GTK.Button get_cancel_button( );
//! Return the cancel button widget.
//!
//!

GTK.ColorSelection get_colorsel( );
//! Return the color selection widget
//!
//!

GTK.Button get_help_button( );
//! Return the help button
//!
//!

GTK.Button get_ok_button( );
//! Return the ok button
//!
//!
