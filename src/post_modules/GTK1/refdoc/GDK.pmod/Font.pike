//! The GdkFont data type represents a font for drawing on the
//! screen. These functions provide support for loading fonts, and also
//! for determining the dimensions of characters and strings when drawn
//! with a particular font.
//! 
//! Fonts in X are specified by a X Logical Font Description. The
//! following description is considerably simplified. For definitive
//! information about XLFD's see the X reference documentation. A X
//! Logical Font Description (XLFD) consists of a sequence of fields
//! separated (and surrounded by) '-' characters. For example, Adobe
//! Helvetica Bold 12 pt, has the full description: 
//! @i{ "-adobe-helvetica-bold-r-normal--12-120-75-75-p-70-iso8859-1"@}
//! 
//!The fields in the XLFD are:
//! @xml{<matrix>@}
//! @xml{<r>@}@xml{<c>@} Foundry@xml{</c>@}
//! @xml{<c>@}the company or organization where the font originated.@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@} Family@xml{</c>@}
//! @xml{<c>@}the font family (a group of related font designs).@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@} Weight@xml{</c>@}
//! @xml{<c>@}A name for the font's typographic weight For example, 'bold' or 'medium').@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@} Slant@xml{</c>@}
//! @xml{<c>@}The slant of the font. Common values are 'R' for Roman, 'I' for italoc, and 'O' for oblique.@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@} Set Width@xml{</c>@}
//! @xml{<c>@}A name for the width of the font. For example, 'normal' or 'condensed'.@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@} Add Style@xml{</c>@}
//! @xml{<c>@}Additional information to distinguish a font from other fonts of the same family.@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@} Pixel Size@xml{</c>@}
//! @xml{<c>@}The body size of the font in pixels.@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@} Point Size@xml{</c>@}
//! @xml{<c>@}The body size of the font in 10ths of a point. (A point is 1/72.27 inch) @xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@} Resolution X@xml{</c>@}
//! @xml{<c>@}The horizontal resolution that the font was designed for.@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@} Resolution Y@xml{</c>@}
//! @xml{<c>@}The vertical resolution that the font was designed for .@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@} Spacing@xml{</c>@}
//! @xml{<c>@}The type of spacing for the font - can be 'p' for proportional, 'm' for monospaced or 'c' for charcell.@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@} Average Width@xml{</c>@}
//! @xml{<c>@}The average width of a glyph in the font. For monospaced and charcell fonts, all glyphs in the font have this width@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@}Charset Registry@xml{</c>@}
//! @xml{<c>@}                          The registration authority that owns the encoding for the font. Together with the Charset Encoding field, this                        defines the character set for the font.@xml{</c>@}@xml{</r>@}
//! @xml{<r>@}@xml{<c>@}Charset Encoding@xml{</c>@}
//! @xml{<c>@}An identifier for the particular character set encoding.@xml{</c>@}@xml{</r>@}
//! @xml{</matrix>@}
//! 
//! When specifying a font via a X logical Font Description, '*' can be
//! used as a wildcard to match any portion of the XLFD. For instance,
//! the above example could also be specified as
//! @i{"-*-helvetica-bold-r-normal--*-120-*-*-*-*-iso8859-1"@}
//! 
//! It is generally a good idea to use wildcards for any portion of the
//! XLFD that your program does not care about specifically, since that
//! will improve the chances of finding a matching font.
//! 
//!
//!

int char_width( int character );
//! Return the width, in pixels, of the specified character, if
//! rendered with this font. The character can be between 0 and 65535,
//! the character encoding is font specific.
//!
//!

static GDK.Font create( string|void font_name );
//! Create a new font object. The string is the font XLFD.
//!
//!

GDK.Font destroy( );
//! Free the font, called automatically by pike when the object is destroyed.
//!
//!
