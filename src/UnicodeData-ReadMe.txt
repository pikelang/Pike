UNICODE CHARACTER DATABASE
Version 3.0.0

 Revision3.0.0
 Authors Mark Davis and Ken Whistler
 Date    1999-09-11
 This    ftp://ftp.unicode.org/Public/3.0-Update/UnicodeCharacterDatabase-3.0.0.html
 Version
 Previousn/a
 Version
 Latest  ftp://ftp.unicode.org/Public/3.0-Update/UnicodeCharacterDatabase-3.0.0.html
 Version

          Copyright © 1995-1999 Unicode, Inc. All Rights reserved.

Disclaimer

The Unicode Character Database is provided as is by Unicode, Inc. No claims
are made as to fitness for any particular purpose. No warranties of any kind
are expressed or implied. The recipient agrees to determine applicability of
information provided. If this file has been purchased on magnetic or optical
media from Unicode, Inc., the sole remedy for any claim will be exchange of
defective media within 90 days of receipt.

This disclaimer is applicable for all other data files accompanying the
Unicode Character Database, some of which have been compiled by the Unicode
Consortium, and some of which have been supplied by other sources.

Limitations on Rights to Redistribute This Data

Recipient is granted the right to make copies in any form for internal
distribution and to freely use the information supplied in the creation of
products supporting the UnicodeTM Standard. The files in the Unicode
Character Database can be redistributed to third parties or other
organizations (whether for profit or not) as long as this notice and the
disclaimer notice are retained. Information can be extracted from these
files and used in documentation or programs, as long as there is an
accompanying notice indicating the source.

Introduction

The Unicode Character Database is a set of files that define the Unicode
character properties and internal mappings. For more information about
character properties and mappings, see The Unicode Standard.

The Unicode Character Database has been updated to reflect Version 3.0 of
the Unicode Standard, with many characters added to those published in
Version 2.0. A number of corrections have also been made to case mappings or
other errors in the database noted since the publication of Version 2.0.
Normative bidirectional properties have also been modified to reflect
decisions of the Unicode Technical Committee.

For more information on versions of the Unicode Standard and how to
reference them, see http://www.unicode.org/unicode/standard/versions/.

Conformance

Character properties may be either normative or informative. Normative means
that implementations that claim conformance to the Unicode Standard (at a
particular version) and which make use of a particular property or field
must follow the specifications of the standard for that property or field in
order to be conformant. The term normative when applied to a property or
field of the Unicode Character Database, does not mean that the value of
that field will never change. Corrections and extensions to the standard in
the future may require minor changes to normative values, even though the
Unicode Technical Committee strives to minimize such changes. An informative
property or field is strongly recommended, but a conformant implementation
is free to use or change such values as it may require while still being
conformant to the standard. Particular implementations may choose to
override the properties and mappings that are not normative. In that case,
it is up to the implementer to establish a protocol to convey that
information.

Files

The following summarizes the files in the Unicode Character Database.  For
more information about these files, see the referenced technical report or
section of Unicode Standard, Version 3.0.

UnicodeData.txt (Chapter 4)

   * The main file in the Unicode Character Database.
   * For detailed information on the format, see UnicodeData.html. This file
     also characterizes which properties are normative and which are
     informative.

PropList.txt (Chapter 4)

   * Additional informative properties list: Alphabetic, Ideographic, and
     Mathematical, among others.

SpecialCasing.txt (Chapter 4)

   * List of informative special casing properties, including one-to-many
     mappings such as SHARP S => "SS", and locale-specific mappings, such as
     for Turkish dotless i.

Blocks.txt (Chapter 14)

   * List of normative block names.

Jamo.txt (Chapter 4)

   * List of normative Jamo short names, used in deriving HANGUL SYLLABLE
     names algorithmically.

ArabicShaping.txt (Section 8.2)

   * Basic Arabic and Syriac character shaping properties, such as initial,
     medial and final shapes. These properties are normative for minimal
     shaping of Arabic and Syriac.

NamesList.txt (Chapter 14)

   * This file duplicates some of the material in the UnicodeData file, and
     adds informative annotations uses in the character charts, as printed
     in the Unicode Standard.
   * Note: The information in NamesList.txt and Index.txt files matches the
     appropriate version of the book. Changes in the Unicode Character
     Database since then may not be reflected in these files, since they are
     primarily of archival interest.

Index.txt (Chapter 14)

   * Informative index to Unicode characters, as printed in the Unicode
     Standard
   * Note: The information in NamesList.txt and Index.txt files matches the
     appropriate version of the book. Changes in the Unicode Character
     Database since then may not be reflected in these files, since they are
     primarily of archival interest.

CompositionExclusions.txt (UTR#15 Unicode Normalization Forms)

   * Normative properties for normalization.

LineBreak.txt (UTR #14: Line Breaking Properties)

   * Normative and informative properties for line breaking. To see which
     properties are informative and which are normative, consult UTR#14.

EastAsianWidth.txt (UTR #11: East Asian Character Width)

   * Informative properties for determining the choice of wide vs. narrow
     glyphs in East Asian contexts.

diffXvY.txt

   * Mechanically-generated informative files containing accumulated
     differences between successive versions of UnicodeData.txt
