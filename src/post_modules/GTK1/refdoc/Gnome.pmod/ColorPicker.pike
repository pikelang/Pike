//! This widget provides color selection facilities to your
//! application. The widget appears as a button which contains a "color
//! swatch" of the currently selected color. When the button is
//! pressed, the widget presents the user with a color selection dialog
//! where the color can be selected.
//! 
//! You can select the color to be displayed in a number of ways:
//! floating point values for the red, green and blue channels,
//! integers in the range 0 to 65,535, or integers in the range 0 to
//! 255, depending on your needs.
//!@expr{ Gnome.ColorPicker();@}
//!@xml{<image>../images/gnome_colorpicker.png</image>@}
//!
//! 
//!
//!
//!  Signals:
//! @b{color_set@}
//! This signal is emitted when the user changes the color on the color
//! selector. The values passed to this signal are the red, green, blue
//! and alpha channels selected in integer form in the range 0 to
//! 65535.
//!
//!

inherit GTK.Button;

static Gnome.ColorPicker create( );
//! Creates a new GNOME color picker widget. This returns a widget in
//! the form of a small button containing a swatch representing the
//! current selected color. When the button is clicked, a
//! color-selection dialog will open, allowing the user to select a
//! color. The swatch will be updated to reflect the new color when the
//! user finishes.
//!
//!

mapping get( );
//! Returns a mapping ([ "d":([ "r":rvalue, "g":gvalue, "b":bvalue,
//! "a":avalue ]), "i8":([ ... ]), "i16":([ .. ]) ]);
//!
//!

Gnome.ColorPicker set_d( float r, float g, float b, float a );
//! Set color shown in the color picker widget using floating point values.
//! The values range between 0.0 and 1.0.
//!
//!

Gnome.ColorPicker set_dither( int dither );
//! Sets whether the picker should dither the color sample or just
//! paint a solid rectangle.
//!
//!

Gnome.ColorPicker set_i16( int r, int g, int b, int a );
//! Set color shown in the color picker widget using integer values.
//! The values range between 0 and 65535.
//!
//!

Gnome.ColorPicker set_i8( int r, int g, int b, int a );
//! Set color shown in the color picker widget using integer values.
//! The values range between 0 and 255.
//!
//!

Gnome.ColorPicker set_title( string title );
//! Sets the title for the color selection dialog
//!
//!

Gnome.ColorPicker set_use_alpha( int use_alpha );
//! Sets whether or not the picker should use the alpha channel.
//!
//!
