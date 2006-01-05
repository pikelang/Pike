//! The color selection widget is, not surprisingly, a widget for
//! interactive selection of colors.  This composite widget lets the
//! user select a color by manipulating RGB (Red, Green, Blue) and HSV
//! (Hue, Saturation, Value) triples. This is done either by adjusting
//! single values with sliders or entries, or by picking the desired
//! color from a hue-saturation wheel/value bar.  Optionally, the
//! opacity of the color can also be set.
//! 
//! The color selection widget currently emits only one signal,
//! "color_changed", which is emitted whenever the current color in the
//! widget changes, either when the user changes it or if it's set
//! explicitly through set_color().
//! 
//!@expr{ GTK2.ColorSelection()@}
//!@xml{<image>../images/gtk2_colorselection.png</image>@}
//!
//! Properties:
//! int current-alpha
//! GDK2.Color current-color
//! int has-opacity-control
//! int has-palette
//!
//!
//!  Signals:
//! @b{color_changed@}
//!

inherit GTK2.Vbox;

static GTK2.ColorSelection create( mapping|void props );
//! Create a new color selection.
//!
//!

int get_current_alpha( );
//! Returns the current alpha value.
//!
//!

mapping get_current_color( );
//!  When you need to query the current color, typically when you've
//!  received a "color_changed" signal, you use this function. The
//!  return value is an array of floats, See the set_color() function
//!  for the description of this array.
//!
//!

int get_has_opacity_control( );
//! Determines whether the colorsel has an opacity control.
//!
//!

int get_has_palette( );
//! Determines whether the color selector has a color palette.
//!
//!

int get_previous_alpha( );
//! Returns the previous alpha value.
//!
//!

mapping get_previous_color( );
//! Returns the original color value.
//!
//!

int is_adjusting( );
//! Gets the current state of the colorsel.
//!
//!

GTK2.ColorSelection set_current_alpha( int alpha );
//! Sets the current opacity to be alpha.  The first time this
//! is called, it will also set the original opacity to be alpha too.
//!
//!

GTK2.ColorSelection set_current_color( mapping color );
//! You can set the current color explicitly by calling this function
//! with an array of colors (floats). The length of the array depends
//! on whether opacity is enabled or not. Position 0 contains the red
//! component, 1 is green, 2 is blue and opacity is at position 3 (only
//! if opacity is enabled, see set_opacity()) All values are between
//! 0 and 65535
//!
//!

GTK2.ColorSelection set_has_opacity_control( int setting );
//! Sets whether or not to use opacity.
//!
//!

GTK2.ColorSelection set_has_palette( int has_palette );
//! Shows and hides the palette based upon the value of has_palette
//!
//!

GTK2.ColorSelection set_previous_alpha( int alpha );
//! Sets the 'previous' alpha to be alpha.  This function should
//! be called with some hesitation, as it might seem confusing
//! to have that alpha change.
//!
//!

GTK2.ColorSelection set_previous_color( mapping colors );
//! Sets the 'previous' color to be color.
//!
//!
