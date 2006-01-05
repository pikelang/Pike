//! Pango Font Description.
//!
//!

int better_match( GTK2.Pango.FontDescription new, GTK2.Pango.FontDescription old );
//! Determines if the style attributes of new are a closer match than old, or
//! if old is omitted, determines if new is a match at all.  Approximate
//! matching is done for weight and style; other attributes must match exactly.
//!
//!

GTK2.Pango.FontDescription copy( );
//! Copy a font description.
//!
//!

static Pango.FontDescription create( string|void desc );
//! Create a new font description.  If desc is present, creates a new font
//! description from a string representation in the form
//! "[FAMILY-LIST] [STYLE-OPTIONS] [SIZE]", where FAMILY-LIST is a comma
//! separated list of families optionally terminated by a comma, STYLE-OPTIONS
//! is a whitespace separated list of words where each word describes one of
//! style, variant, weight, or stretch, and size is a decimal number (size in
//! points).  Any one of the options may be absent.  If FAMILY-LIST is absent,
//! then the family name will be blank.  If STYLE-OPTIONS is missing, then all
//! style options will be set to the default values.  If SIZE is missing, the
//! size in the resulting font description will be set to 0.
//!
//!

Pango.FontDescription destroy( );
//! Destructor.
//!
//!

int equal( GTK2.Pango.FontDescription desc );
//! Compares two font descriptions for equality.
//!
//!

string get_family( );
//! Gets the family name.
//!
//!

int get_size( );
//! Gets the size.
//!
//!

int get_stretch( );
//! Get the stretch.
//!
//!

int get_style( );
//! Gets the style.
//!
//!

int get_variant( );
//! Gets the variant.
//!
//!

int get_weight( );
//! Gets the weight.
//!
//!

Pango.FontDescription merge( GTK2.Pango.FontDescription desc, int replace );
//! Merges the fields that are set int desc to the fields in this description.
//! If replace is false, only fields in this description that are not already
//! set are affected.  If true, then fields that are already set will be 
//! replaced as well.
//!
//!

Pango.FontDescription set_family( string family );
//! Sets the family name.  The family name represents a family of related
//! fonts styles, and will resolve to a particular PangoFontFamily.
//!
//!

Pango.FontDescription set_size( int size );
//! Sets the size in fractional points.  This is the size of the font in
//! points, scaled by PANGO_SCALE.  (That is, a size value of 10*PANGO_SCALE)
//! is a 10 point font.  The conversion factor between points and device units
//! depends on system configuration and the output device.  For screen display,
//! a logical DPI of 96 is common, in which case a 10 point font corresponds
//! to a 1o*(96.72) = 13.3 pixel font.  Use set_absolute_size() if you need
//! a particular size in device units.
//!
//!

Pango.FontDescription set_stretch( int stretch );
//! Sets the stretch.  This specifies how narrow or wide the font should be.
//! One of @[PANGO_STRETCH_CONDENSED], @[PANGO_STRETCH_EXPANDED], @[PANGO_STRETCH_EXTRA_CONDENSED], @[PANGO_STRETCH_EXTRA_EXPANDED], @[PANGO_STRETCH_NORMAL], @[PANGO_STRETCH_SEMI_CONDENSED], @[PANGO_STRETCH_SEMI_EXPANDED], @[PANGO_STRETCH_ULTRA_CONDENSED] and @[PANGO_STRETCH_ULTRA_EXPANDED].
//!
//!

Pango.FontDescription set_style( int style );
//! Sets the style.  This describes whether the font is slanted and the manner
//! in which is is slanted.  One of @[PANGO_STYLE_ITALIC], @[PANGO_STYLE_NORMAL] and @[PANGO_STYLE_OBLIQUE].  Most fonts will
//! either have an italic style or an oblique style, but not both, and font
//! matching in Pango will match italic specifications with oblique fonts and
//! vice-versa if an exact match is not found.
//!
//!

Pango.FontDescription set_variant( int variant );
//! Sets the variant.  One of @[PANGO_VARIANT_NORMAL] and @[PANGO_VARIANT_SMALL_CAPS].
//!
//!

Pango.FontDescription set_weight( int weight );
//! Sets the weight.  The weight specifies how bold or light the font should
//! be.  In addition to the values of @[PANGO_WEIGHT_BOLD], @[PANGO_WEIGHT_HEAVY], @[PANGO_WEIGHT_LIGHT], @[PANGO_WEIGHT_NORMAL], @[PANGO_WEIGHT_ULTRABOLD] and @[PANGO_WEIGHT_ULTRALIGHT], other intermediate
//! numeric values are possible.
//!
//!

string to_filename( );
//! Creates a filename representation.  The filename is identical to the
//! result from calling to_string(), but with underscores instead of characters
//! that are untypical in filenames, and in lower case only.
//!
//!

string to_string( );
//! Creates a string representation.  The family list in the string description
//! will only have a terminating comm if the last word of the list is a valid
//! style option.
//!
//!
