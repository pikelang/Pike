//
// $Id: Types.pmod,v 1.37 2005/01/21 17:22:45 mast Exp $
//

//! Encodes various asn.1 objects according to the Distinguished
//! Encoding Rules (DER)

#pike __REAL_VERSION__
#pragma strict_types
#define COMPATIBILITY

#if constant(Gmp) && constant(Gmp.mpz)

#if 0
#define WERROR werror
#else
#define WERROR(x ...)
#endif


// Helper functions

#define MAKE_COMBINED_TAG(cls, tag) (((tag) << 2) | (cls))

//! Combines tag and class as a single integer, in a somewhat arbitrary
//! way. This works also for tags beyond 31 (although not for tags
//! beyond 2^30.
//!
//! @param cls
//!   ASN1 type class
//! @param tag
//!   ASN1 type tag
//! @returns
//!   combined tag
//! @seealso
//!  @[Standards.ASN1.Types.extract_tag]
//!  @[Standards.ASN1.Types.extract_cls]
int make_combined_tag(int cls, int tag)
{ return MAKE_COMBINED_TAG(cls, tag); }

//! extract ASN1 type tag from a combined tag
//! @seealso
//!  @[Standards.ASN1.Types.make_combined_tag]
int extract_tag(int i) { return i >> 2; }

//! extract ASN1 type class from a combined tag
//! @seealso
//!  @[Standards.ASN1.Types.make_combined_tag]
int extract_cls(int i) { return i & 3; }


// Class definitions

//! Generic, abstract base class for ASN1 data types.
class Object
{
  constant cls = 0;
  constant tag = 0;
  constant constructed = 0;

  constant type_name = 0;
  string der_encode();

  //! Get the class of this object.
  //!
  //! @returns
  //!   The class of this object.
  int get_cls() { return cls; }

  //! Get the tag for this object.
  //!
  //! @returns
  //!   The tag for this object.
  int get_tag() { return tag; }

  //! Get the combined tag (tag + class) for this object.
  //!
  //! @returns
  //!   the combined tag header
  int get_combined_tag() {
    return make_combined_tag(get_tag(), get_cls());
  }

  string der;

  // Should be overridden by subclasses
  this_program decode_primitive(string contents);
  this_program begin_decode_constructed(string raw);
  this_program decode_constructed_element(int i, object e);
  this_program end_decode_constructed(int length);

  mapping(int:program(Object)) element_types(int i,
      mapping(int:program(Object)) types) {
    return types;
  }
  this_program init(mixed ... args) { return this; }

  string to_base_128(int n) {
    if (!n)
      return "\0";
    /* Convert tag number to base 128 */
    array(int) digits = ({ });

    /* Array is built in reverse order, least significant digit first */
    while(n)
    {
      digits += ({ (n & 0x7f) | 0x80 });
      n >>= 7;
    }
    digits[0] &= 0x7f;

    return sprintf("%@c", reverse(digits));
  }

  string encode_tag() {
    int tag = get_tag();
    int cls = get_cls();
    if (tag < 31)
      return sprintf("%c", (cls << 6) | (constructed << 5) | tag);

    return sprintf("%c%s", (cls << 6) | (constructed << 5) | 0x1f,
		   to_base_128(tag) );
  }

  string encode_length(int|object len) {
    if (len < 0x80)
      return sprintf("%c", len);
    string s = Gmp.mpz(len)->digits(256);
    if (sizeof(s) >= 0x80)
      error("Max length exceeded.\n" );
    return sprintf("%c%s", sizeof(s) | 0x80, s);
  }

  string build_der(string contents) {
    string data = encode_tag() + encode_length(sizeof(contents)) + contents;
    WERROR("build_der: %O\n", data);
    return data;
  }

  string record_der(string s) {
    return (der = s);
  }

  string record_der_contents(string s) {
    record_der(build_der(s));
  }


  //! Get the DER encoded version of this object.
  //!
  //! @returns
  //!   DER encoded representation of this object.
  string get_der() {
    return der || (record_der(der_encode()));
  }

  void create(mixed ...args) {
    WERROR("asn1_object[%s]->create\n", type_name);
    if (sizeof(args))
      init(@args);
  }
}

//! Compound object primitive
class Compound
{
  inherit Object;

  constant constructed = 1;

  //! contents of compound object, elements are from @[Standards.ASN1.Types]
  array(Object) elements = ({ });

  this_program init(array args) {
    WERROR("asn1_compound[%s]->init(%O)\n", type_name, args);
    foreach(args, mixed o)
      if (!objectp(o))
	error( "Non-object argument!\n" );
    elements = [array(Object)]args;
    WERROR("asn1_compound: %O\n", elements);
    return this;
  }

  this_program begin_decode_constructed(string raw) {
    WERROR("asn1_compound[%s]->begin_decode_constructed\n", type_name);
    record_der_contents(raw);
    return this;
  }

  this_program decode_constructed_element(int i, object e) {
    WERROR("asn1_compound[%s]->decode_constructed_element(%O)\n",
	   type_name, e);
    if (i != sizeof(elements))
      error("Unexpected index!\n");
    elements += ({ e });
    return this;
  }

  this_program end_decode_constructed(int length) {
    if (length != sizeof(elements))
      error("Invalid length!\n");
    return this;
  }

  static string _sprintf(int t) {
    return t=='O' && sprintf("%O(%s %O)", this_program, type_name, elements);
  }

#ifdef COMPATIBILITY
  string debug_string() {
    WERROR("asn1_compound[%s]->debug_string(), elements = %O\n",
	   type_name, elements);
    return _sprintf('O');
  }
#endif
}

//! string object primitive
class String
{
  inherit Object;

  //! value of object
  string value;

  this_program init(string s) {
    value = s;
    return this;
  }

  string der_encode() {
    return build_der(value);
  }

  this_program decode_primitive(string contents) {
    record_der_contents(contents);
    value = contents;
    return this;
  }

  static string _sprintf(int t) {
    return t=='O' && sprintf("%O(%s, %O)", this_program, type_name, value);
  }

#ifdef COMPATIBILITY
  string debug_string() {
    WERROR("asn1_string[%s]->debug_string(), value = %O\n", type_name, value);
    return _sprintf('O');
  }
#endif
}

// FIXME: What is the DER-encoding of TRUE???
// According to Jan Petrous, the LDAP spec says that 0xff is right.
// No, every nonzero value is true according to the ASN1 spec,
// but they use 0xff as example of a true value. /Nilsson

//! boolean object
class Boolean
{
  inherit Object;
  constant tag = 1;
  constant type_name = "BOOLEAN";

  //! value of object
  int value;

  this_program init(int x) { value = x; return this; }

  string der_encode() { return build_der(value ? "\377" : "\0"); }

  this_program decode_primitive(string contents) {
    if (sizeof(contents) != 1)
    {
      WERROR("asn1_boolean->decode_primitive: Bad length.\n");
      return 0;
    }
    record_der_contents(contents);
    value = contents[0];
    return this;
  }

  static string _sprintf(int t) {
    return t=='O' && sprintf("%O(%s)", this_program, (value?"TRUE":"FALSE"));
  }

#ifdef COMPATIBILITY
  string debug_string() {
    return value ? "TRUE" : "FALSE";
  }
#endif
}

//! Integer object
//! All integers are represented as bignums, for simplicity
class Integer
{
  inherit Object;
  constant tag = 2;
  constant type_name = "INTEGER";

  //! value of object
  Gmp.mpz value;

  this_object init(int|object n) {
    value = Gmp.mpz(n);
    WERROR("i = %s\n", value->digits());
    return this;
  }

  string der_encode() {
    string s;

    if (value < 0)
    {
      Gmp.mpz n = [object(Gmp.mpz)](value +
				   pow(256, ([object(Gmp.mpz)](- value))->
				       size(256)));
      s = n->digits(256);
      if (!(s[0] & 0x80))
	s = "\377" + s;
    } else {
      s = value->digits(256);
      if (s[0] & 0x80)
	s = "\0" + s;
    }
    return build_der(s);
  }

  this_object decode_primitive(string contents) {
    record_der_contents(contents);
    value = Gmp.mpz(contents, 256);
    if (contents[0] & 0x80)  /* Negative */
      value -= pow(256, sizeof(contents));
    return this;
  }

  static string _sprintf(int t) {
    return t=='O' && sprintf("%O(%d %s)", this_program,
			     value->size(), value->digits());
  }

#ifdef COMPATIBILITY
  string debug_string() {
    return sprintf("INTEGER (%d) %s", value->size(), value->digits());
  }
#endif
}

//! Enumerated object
class Enumerated
{
  inherit Integer;
  constant tag = 10;
  constant type_name ="ENUMERATED";
}

//! Bit string object
class BitString
{
  inherit Object;
  constant tag = 3;
  constant type_name = "BIT STRING";

  //! value of object
  string value;

  int unused = 0;

  this_program init(string s) { value = s; return this; }

  string der_encode() {
    return build_der(sprintf("%c%s", unused, value));
  }

  //! Set the bitstring value as a string with @expr{"1"@} and
  //! @expr{"0"@}.
  this_program set_from_ascii(string s) {
    array v = array_sscanf(s, "%8b"*(sizeof(s)/8)+"%b");
    v[-1] = v[-1]<<((-sizeof(s))%8);
    value = (string)v;
    set_length(sizeof(s));
    return this;
  }

  //! Sets the length of the bit string to @[len] number of bits.
  int set_length(int len) {
    if (len)
    {
      value = value[..(len + 7)/8];
      unused = (- len) % 8;
      value = sprintf("%s%c", value[..sizeof(value)-2], value[-1]
		      & ({ 0xff, 0xfe, 0xfc, 0xf8,
			   0xf0, 0xe0, 0xc0, 0x80 })[unused]);
    } else {
      unused = 0;
      value = "";
    }
  }

  this_program decode_primitive(string contents) {
    record_der_contents(contents);
    if (!sizeof(contents))
      return 0;
    unused = contents[0];

    if (unused >= 8)
      return 0;
    value = contents[1..];
    return this;
  }

  static string _sprintf(int t) {
    int size = sizeof(value)*8-unused;
    return t=='O' && sprintf("%O(%d %0"+size+"s)", this_program, size,
			     ([object(Gmp.mpz)](Gmp.mpz(value, 256) >> unused))
			     ->digits(2));
  }

#ifdef COMPATIBILITY
  string debug_string() {
    return sprintf("BIT STRING (%d) %s",
		   sizeof(value) * 8 - unused,
		   ([object(Gmp.mpz)](Gmp.mpz(value, 256) >> unused))
		   ->digits(2));
  }
#endif
}

//! Octet string object
class OctetString
{
  inherit String;
  constant tag = 4;
  constant type_name = "OCTET STRING";
}

//! Null object
class Null
{
  inherit Object;
  constant tag = 5;
  constant type_name = "NULL";

  string der_encode() { return build_der(""); }

  this_program decode_primitive(string contents) {
    record_der_contents(contents);
    return !sizeof(contents) && this;
  }

#ifdef COMPATIBILITY
  string debug_string() { return "NULL"; }
#endif
}

//! Object identifier object
class Identifier
{
  inherit Object;
  constant tag = 6;
  constant type_name = "OBJECT IDENTIFIER";

  //! value of object
  array(int) id;

  this_program init(int ... args) {
    if ( (sizeof(args) < 2)
	 || (args[0] > 2)
	 || (args[1] >= ( (args[0] < 2) ? 40 : 176) ))
      error( "Invalid object identifier.\n" );
    id = args;
    return this;
  }

  mixed _encode() { return id; }
  void _decode(array(int) data) { id=data; }

  //! Returns a new @[Identifier] object with @[args] appended to the
  //! ID path.
  this_program append(int ... args) {
    return this_program(@id, @args);
  }

  string der_encode() {
    return build_der(sprintf("%s%@s", to_base_128(40 * id[0] + id[1]),
			     map(id[2..], to_base_128)));
  }

  this_program decode_primitive(string contents) {
    record_der_contents(contents);

    if (contents[0] < 120)
      id = ({ contents[0] / 40, contents[0] % 40 });
    else
      id = ({ 2, contents[0] - 80 });
    int index = 1;
    while(index < sizeof(contents))
    {
      int element = 0;
      do {
	element = element << 7 | (contents[index] & 0x7f);
      } while(contents[index++] & 0x80);
      id += ({ element });
    }
    return this;
  }

  static string _sprintf(int t) {
    return t=='O' && sprintf("%O(%s)", this_program, (array(string))id*".");
  }

#ifdef COMPATIBILITY
  string debug_string() {
    return "IDENTIFIER " + (array(string)) id * ".";
  }
#endif

  int __hash()
  {
    return hash(get_der());
  }

  int `==(mixed other) {
    return (objectp(other) &&
	    (this_program == object_program(other)) &&
	    equal(id, ([object(Identifier)]other)->id));
  }
}

//! Checks if a Pike string can be encoded with UTF8. That is
//! always the case...
int(1..1) asn1_utf8_valid (string s)
{
  return 1;
}

//! UTF8 string object
//!
//! Character set: ISO/IEC 10646-1 (compatible with Unicode).
//!
//! Variable width encoding, see rfc2279.
class UTF8String
{
  inherit String;
  constant tag = 12;
  constant type_name = "UTF8String";

  string der_encode() {
    return build_der(string_to_utf8(value));
  }

  this_program decode_primitive(string contents) {
    record_der(contents);
    if (catch {
      value = utf8_to_string(contents);
    })
      return 0;

    return this;
  }
}

//! Sequence object
class Sequence
{
  inherit Compound;
  constant tag = 16;
  constant type_name = "SEQUENCE";

  string der_encode() {
    WERROR("ASN1.Sequence: elements = '%O\n", elements);
    array(string) a = elements->get_der();
    WERROR("ASN1.Sequence: der_encode(elements) = '%O\n", a);
    return build_der(`+("", @ a));
  }
}

//! Set object
class Set
{
  inherit Compound;
  constant tag = 17;
  constant type_name = "SET";

  int(-1..1) compare_octet_strings(string r, string s) {
    for(int i = 0;; i++) {
      if (i == sizeof(r))
	return (i = sizeof(s)) ? 0 : 1;
      if (i == sizeof(s))
	return -1;
      if (r[i] < s[i])
	return 1;
      else if (r[i] > s[i])
	return -1;
    }
  }

  string der_encode() {
    WERROR("asn1_set->der: elements = '%O\n", elements);
    array(string) a = elements->get_der();
    WERROR("asn1_set->der: der_encode(elements) = '%O\n", a);
    return build_der(`+("", @[array(string)]
			Array.sort_array(a, compare_octet_strings)));
  }
}

Regexp asn1_printable_invalid_chars = Regexp("([^-A-Za-z0-9 '()+,./:=?])");

//! Checks if a Pike string can be encoded as a @[PrintableString].
int(0..1) asn1_printable_valid (string s) {
  if (global.String.width(s)!=8) return 0;
  return !asn1_printable_invalid_chars->match(s);
}

//! PrintableString object
class PrintableString
{
  inherit String;
  constant tag = 19;
  constant type_name = "PrintableString";
}

Regexp asn1_teletex_invalid_chars = Regexp ("([\\\\{}\240®©¬¦\255Ð])");

//!
int(0..1) asn1_teletex_valid (string s)
{
  if (global.String.width(s)!=8)
    // T.61 encoding of wide strings not implemented.
    return 0;
  return !asn1_teletex_invalid_chars->match (s);
}

//! TeletexString object
//!
//! Avoid this one; it seems to be common that this type is used to
//! label strings encoded with the ISO 8859-1 character set (use
//! asn1_broken_teletex_string for that). From
//! http://www.mindspring.com/~asn1/nlsasn.htm:
//!
//! /.../ Types GeneralString, VideotexString, TeletexString
//! (T61String), and GraphicString exist in earlier versions
//! [pre-1994] of ASN.1. They are considered difficult to use
//! correctly by applications providing national language support.
//! Varying degrees of application support for T61String values seems
//! to be most common in older applications. Correct support is made
//! more difficult, as character values available in type T61String
//! have changed with the addition of new register entries from the
//! 1984 through 1997 versions.
//!
//! This implementation is based on the description of T.61 and T.51
//! in "Some functions for representing T.61 characters from the
//! X.500 Directory Service in ISO 8859 terminals (Version 0.2. July
//! 1994.)" by Enrique Silvestre Mora (mora@@si.uji.es), Universitat
//! Jaume I, Spain, found in the package
//! ftp://pereiii.uji.es/pub/uji-ftp/unix/ldap/iso-t61.translation.tar.Z
//!
//! The translation is only complete for 8-bit latin 1 strings. It
//! encodes strictly to T.61, but decodes from the superset T.51.
class TeletexString
{
  inherit String;
  constant tag = 20;
  constant type_name = "TeletexString";	// Alias: T61String

#define ENC_ERR(char) char
#define DEC_ERR(str) str
#define DEC_COMB_MARK "\300"

#define GR(char) "\301" char	/* Combining grave accent */
#define AC(char) "\302" char	/* Combining acute accent */
#define CI(char) "\303" char	/* Combining circumflex accent */
#define TI(char) "\304" char	/* Combining tilde */
#define MA(char) "\305" char	/* Combining macron */
#define BR(char) "\306" char	/* Combining breve */
#define DA(char) "\307" char	/* Combining dot above */
#define DI(char) "\310" char	/* Combining diaeresis */
#define RA(char) "\312" char	/* Combining ring above */
#define CE(char) "\313" char	/* Combining cedilla */
#define UN(char) "\314" char	/* Combining underscore (note 6) */
#define DO(char) "\315" char	/* Combining double acute accent */
#define OG(char) "\316" char	/* Combining ogonek */
#define CA(char) "\317" char	/* Combining caron */

  constant encode_from = ({
    /*"#", "$",*/ "¤",		// Note 3
    "\\", "{", "}",		// Note 7
    "\240",			// No-break space (note 7)
    "×",			// Multiplication sign
    "÷",			// Division sign
    "¹",			// Superscript one
    "®",			// Registered sign (note 7)
    "©",			// Copyright sign (note 7)
    "¬",			// Not sign (note 7)
    "¦",			// Broken bar (note 7)
    "Æ",			// Latin capital ligature ae
    "ª",			// Feminine ordinal indicator
    "Ø",			// Latin capital letter o with stroke
    "º",			// Masculine ordinal indicator
    "Þ",			// Latin capital letter thorn
    "æ",			// Latin small ligature ae
    "ð",			// Latin small letter eth
    "ø",			// Latin small letter o with stroke
    "ß",			// Latin small letter sharp s
    "þ",			// Latin small letter thorn
    "\255",			// Soft hyphen (note 7)
    "Ð",			// Latin capital letter eth (no equivalent)
    // Combinations
    "^", "`", "~",		// Note 4
    "´", "¨", "¯", "¸",
    "À", "Á", "Â", "Ã", "Ä", "Å",
    "à", "á", "â", "ã", "ä", "å",
    "Ç",
    "ç",
    "È", "É", "Ê", "Ë",
    "è", "é", "ê", "ë",
    "Ì", "Í", "Î", "Ï",
    "ì", "í", "î", "ï",
    "Ñ",
    "ñ",
    "Ò", "Ó", "Ô", "Õ", "Ö",
    "ò", "ó", "ô", "õ", "ö",
    "Ù", "Ú", "Û", "Ü",
    "ù", "ú", "û", "ü",
    "Ý",
    "ý",
  });

  constant encode_to = ({
    /*"#", "$",*/ "\250",	// Note 3
    ENC_ERR("\\"), ENC_ERR("{"), ENC_ERR("}"), // Note 7
    ENC_ERR("\240"),		// No-break space (note 7)
    "\264",			// Multiplication sign
    "\270",			// Division sign
    "\321",			// Superscript one
    ENC_ERR("®"),		// Registered sign (note 7)
    ENC_ERR("©"),		// Copyright sign (note 7)
    ENC_ERR("¬"),		// Not sign (note 7)
    ENC_ERR("¦"),		// Broken bar (note 7)
    "\341",			// Latin capital ligature ae
    "\343",			// Feminine ordinal indicator
    "\351",			// Latin capital letter o with stroke
    "\353",			// Masculine ordinal indicator
    "\354",			// Latin capital letter thorn
    "\361",			// Latin small ligature ae
    "\363",			// Latin small letter eth
    "\371",			// Latin small letter o with stroke
    "\373",			// Latin small letter sharp s
    "\374",			// Latin small letter thorn
    ENC_ERR("\255"),		// Soft hyphen (note 7)
    ENC_ERR("Ð"),		// Latin capital letter eth (no equivalent)
    // Combinations
    CI(" "), GR(" "), TI(" "),	// Note 4
    AC(" "), DI(" "), MA(" "), CE(" "),
    GR("A"), AC("A"), CI("A"), TI("A"), DI("A"), RA("A"),
    GR("a"), AC("a"), CI("a"), TI("a"), DI("a"), RA("a"),
    CE("C"),
    CE("c"),
    GR("E"), AC("E"), CI("E"), DI("E"),
    GR("e"), AC("e"), CI("e"), DI("e"),
    GR("I"), AC("I"), CI("I"), DI("I"),
    GR("i"), AC("i"), CI("i"), DI("i"),
    TI("N"),
    TI("n"),
    GR("O"), AC("O"), CI("O"), TI("O"), DI("O"),
    GR("o"), AC("o"), CI("o"), TI("o"), DI("o"),
    GR("U"), AC("U"), CI("U"), DI("U"),
    GR("u"), AC("u"), CI("u"), DI("u"),
    GR("Y"),
    GR("y"),
  });

  constant decode_from = ({
    /*"#", "$",*/ "\244", "\246", "\250", // Note 3
    /*"^", "`", "~",*/		// Note 4
    "\251",			// Left single quotation mark (note 7)
    "\252",			// Left double quotation mark (note 7)
    "\254",			// Leftwards arrow (note 7)
    "\255",			// Upwards arrow (note 7)
    "\256",			// Rightwards arrow (note 7)
    "\257",			// Downwards arrow (note 7)
    "\264",			// Multiplication sign
    "\270",			// Division sign
    "\271",			// Right single quotation mark (note 7)
    "\272",			// Right double quotation mark (note 7)
    "\300",			// Note 5
    GR(""),			// Combining grave accent
    AC(""),			// Combining acute accent
    CI(""),			// Combining circumflex accent
    TI(""),			// Combining tilde
    MA(""),			// Combining macron
    BR(""),			// Combining breve
    DA(""),			// Combining dot above
    DI(""),			// Combining diaeresis
    "\311",			// Note 5
    RA(""),			// Combining ring above
    CE(""),			// Combining cedilla
    UN(""),			// Combining underscore (note 6)
    DO(""),			// Combining double acute accent
    OG(""),			// Combining ogonek
    CA(""),			// Combining caron
    "\320",			// Em dash (note 7)
    "\321",			// Superscript one
    "\322",			// Registered sign (note 7)
    "\323",			// Copyright sign (note 7)
    "\324",			// Trade mark sign (note 7)
    "\325",			// Eighth note (note 7)
    "\326",			// Not sign (note 7)
    "\327",			// Broken bar (note 7)
    "\330", "\331", "\332", "\333", // Note 2
    "\334",			// Vulgar fraction one eighth (note 7)
    "\335",			// Vulgar fraction three eighths (note 7)
    "\336",			// Vulgar fraction five eighths (note 7)
    "\337",			// Vulgar fraction seven eighths (note 7)
    "\340",			// Ohm sign
    "\341",			// Latin capital ligature ae
    "\342",			// Latin capital letter d with stroke
    "\343",			// Feminine ordinal indicator
    "\344",			// Latin capital letter h with stroke
    "\345",			// Note 2
    "\346",			// Latin capital ligature ij
    "\347",			// Latin capital letter l with middle dot
    "\350",			// Latin capital letter l with stroke
    "\351",			// Latin capital letter o with stroke
    "\352",			// Latin capital ligature oe
    "\353",			// Masculine ordinal indicator
    "\354",			// Latin capital letter thorn
    "\355",			// Latin capital letter t with stroke
    "\356",			// Latin capital letter eng
    "\357",			// Latin small letter n preceded by apostrophe
    "\360",			// Latin small letter kra
    "\361",			// Latin small ligature ae
    "\362",			// Latin small letter d with stroke
    "\363",			// Latin small letter eth
    "\364",			// Latin small letter h with stroke
    "\365",			// Latin small letter dotless i
    "\366",			// Latin small ligature ij
    "\367",			// Latin small letter l with middle dot
    "\370",			// Latin small letter l with stroke
    "\371",			// Latin small letter o with stroke
    "\372",			// Latin small ligature oe
    "\373",			// Latin small letter sharp s
    "\374",			// Latin small letter thorn
    "\375",			// Latin small letter t with stroke
    "\376",			// Latin small letter eng
    "\377",			// Soft hyphen (note 7)
  });

  constant decode_to = ({
    /*"#", "$",*/ "$", "#", "\244", // Note 3
    /*"^", "`", "~",*/		// Note 4
    DEC_ERR("\251"),		// Left single quotation mark (note 7)
    DEC_ERR("\252"),		// Left double quotation mark (note 7)
    DEC_ERR("\254"),		// Leftwards arrow (note 7)
    DEC_ERR("\255"),		// Upwards arrow (note 7)
    DEC_ERR("\256"),		// Rightwards arrow (note 7)
    DEC_ERR("\257"),		// Downwards arrow (note 7)
    "×",			// Multiplication sign
    "÷",			// Division sign
    DEC_ERR("\271"),		// Right single quotation mark (note 7)
    DEC_ERR("\272"),		// Right double quotation mark (note 7)
    DEC_ERR("\300"),		// Note 5
    DEC_COMB_MARK GR(""),	// Combining grave accent
    DEC_COMB_MARK AC(""),	// Combining acute accent
    DEC_COMB_MARK CI(""),	// Combining circumflex accent
    DEC_COMB_MARK TI(""),	// Combining tilde
    DEC_COMB_MARK MA(""),	// Combining macron
    DEC_COMB_MARK BR(""),	// Combining breve
    DEC_COMB_MARK DA(""),	// Combining dot above
    DEC_COMB_MARK DI(""),	// Combining diaeresis
    DEC_ERR("\311"),		// Note 5
    DEC_COMB_MARK RA(""),	// Combining ring above
    DEC_COMB_MARK CE(""),	// Combining cedilla
    DEC_COMB_MARK UN(""),	// Combining underscore (note 6)
    DEC_COMB_MARK DO(""),	// Combining double acute accent
    DEC_COMB_MARK OG(""),	// Combining ogonek
    DEC_COMB_MARK CA(""),	// Combining caron
    DEC_ERR("\320"),		// Em dash (note 7)
    "¹",			// Superscript one
    "®",			// Registered sign (note 7)
    "©",			// Copyright sign (note 7)
    DEC_ERR("\324"),		// Trade mark sign (note 7)
    DEC_ERR("\325"),		// Eighth note (note 7)
    "¬",			// Not sign (note 7)
    "¦",			// Broken bar (note 7)
    DEC_ERR("\330"), DEC_ERR("\331"), DEC_ERR("\332"), DEC_ERR("\333"), // Note 2
    DEC_ERR("\334"),		// Vulgar fraction one eighth (note 7)
    DEC_ERR("\335"),		// Vulgar fraction three eighths (note 7)
    DEC_ERR("\336"),		// Vulgar fraction five eighths (note 7)
    DEC_ERR("\337"),		// Vulgar fraction seven eighths (note 7)
    DEC_ERR("\340"),		// Ohm sign
    "Æ",			// Latin capital ligature ae
    DEC_ERR("\342"),		// Latin capital letter d with stroke
    "ª",			// Feminine ordinal indicator
    DEC_ERR("\344"),		// Latin capital letter h with stroke
    DEC_ERR("\345"),		// Note 2
    DEC_ERR("\346"),		// Latin capital ligature ij
    DEC_ERR("\347"),		// Latin capital letter l with middle dot
    DEC_ERR("\350"),		// Latin capital letter l with stroke
    "Ø",			// Latin capital letter o with stroke
    DEC_ERR("\352"),		// Latin capital ligature oe
    "º",			// Masculine ordinal indicator
    "Þ",			// Latin capital letter thorn
    DEC_ERR("\355"),		// Latin capital letter t with stroke
    DEC_ERR("\356"),		// Latin capital letter eng
    DEC_ERR("\357"),		// Latin small letter n preceded by apostrophe
    DEC_ERR("\360"),		// Latin small letter kra
    "æ",			// Latin small ligature ae
    DEC_ERR("\362"),		// Latin small letter d with stroke
    "ð",			// Latin small letter eth
    DEC_ERR("\364"),		// Latin small letter h with stroke
    DEC_ERR("\365"),		// Latin small letter dotless i
    DEC_ERR("\366"),		// Latin small ligature ij
    DEC_ERR("\367"),		// Latin small letter l with middle dot
    DEC_ERR("\370"),		// Latin small letter l with stroke
    "ø",			// Latin small letter o with stroke
    DEC_ERR("\372"),		// Latin small ligature oe
    "ß",			// Latin small letter sharp s
    "þ",			// Latin small letter thorn
    DEC_ERR("\375"),		// Latin small letter t with stroke
    DEC_ERR("\376"),		// Latin small letter eng
    "\255",			// Soft hyphen (note 7)
  });

  constant decode_comb = ([
    GR(" "): "`",
    AC(" "): "´",
    CI(" "): "^",
    TI(" "): "~",
    DI(" "): "¨",
    // RA(" "): DEC_ERR(RA(" ")),
    MA(" "): "¯",
    // BR(" "): DEC_ERR(BR(" ")),
    // DA(" "): DEC_ERR(DA(" ")),
    CE(" "): "¸",
    // DO(" "): DEC_ERR(DO(" ")),
    // OG(" "): DEC_ERR(OG(" ")),
    // CA(" "): DEC_ERR(CA(" ")),
    GR("A"): "À", AC("A"): "Á", CI("A"): "Â", TI("A"): "Ã", DI("A"): "Ä", RA("A"): "Å",
    GR("a"): "à", AC("a"): "á", CI("a"): "â", TI("a"): "ã", DI("a"): "ä", RA("a"): "å",
    CE("C"): "Ç",
    CE("c"): "ç",
    GR("E"): "È", AC("E"): "É", CI("E"): "Ê", DI("E"): "Ë",
    GR("e"): "è", AC("e"): "é", CI("e"): "ê", DI("e"): "ë",
    GR("I"): "Ì", AC("I"): "Í", CI("I"): "Î", DI("I"): "Ï",
    GR("i"): "ì", AC("i"): "í", CI("i"): "î", DI("i"): "ï",
    TI("N"): "Ñ",
    TI("n"): "ñ",
    GR("O"): "Ò", AC("O"): "Ó", CI("O"): "Ô", TI("O"): "Õ", DI("O"): "Ö",
    GR("o"): "ò", AC("o"): "ó", CI("o"): "ô", TI("o"): "õ", DI("o"): "ö",
    GR("U"): "Ù", AC("U"): "Ú", CI("U"): "Û", DI("U"): "Ü",
    GR("u"): "ù", AC("u"): "ú", CI("u"): "û", DI("u"): "ü",
    GR("Y"): "Ý",
    GR("y"): "ý",
  ]);

  /* Notes from Moras paper:

     (1) All characters in 0xC0-0xCF are non-spacing characters.  They are
     all diacritical marks.  To be represented stand-alone, they need to
     be followed by a SPACE (0x20).  They can appear, also, before
     letters if the couple is one of the defined combinations.

     (2) Reserved for future standardization.

     (3) Current terminals may send and receive 0xA6 and 0xA4 for the NUMBER
     SIGN and DOLLAR SIGN, respectively.  When receiving codes 0x23 and
     0x24, they may interpret them as NUMBER SIGN and CURRENCY SIGN,
     respectively.  Future applications should code the NUMBER SIGN,
     DOLLAR SIGN and CURRENCY SIGN as 0x23, 0x24 and 0xA8, respectively.

     (4) Terminals should send only the codes 0xC1, 0xC3 and 0xC4, followed
     by SPACE (0x20) for stand-alone GRAVE ACCENT, CIRCUMFLEX ACCENT and
     TILDE, respectively.  Nevertheless the terminal shall interpret the
     codes 0x60, 0x5E and 0x7E as GRAVE, CIRCUMFLEX and TILDE,
     respectively.

     (5) This code position is reserved and shall not be used.

     (6) It is recommended to implement the "underline" function by means of
     the control function SGR(4) instead of the "non-spacing underline"
     graphic character.

     (7) Not used in current teletex service (Recommendation T.61).
  */

#undef GR
#undef AC
#undef CI
#undef TI
#undef MA
#undef BR
#undef DA
#undef DI
#undef RA
#undef CE
#undef UN
#undef DO
#undef OG
#undef CA

  string der_encode() {
    return build_der(replace(value, [array(string)]encode_from,
			     [array(string)]encode_to));
  }

  this_program decode_primitive (string contents) {
    record_der (contents);

    array(string) parts =
      replace (contents, [array(string)]decode_from,
	       [array(string)]decode_to) / DEC_COMB_MARK;
    value = parts[0];
    foreach (parts[1..], string part)
      value += (decode_comb[part[..1]] || DEC_ERR(part[..1])) + part[2..];

    return this;
  }

#undef ENC_ERR
#undef DEC_ERR
#undef DEC_COMB_MARK
}

//!
int(0..1) asn1_broken_teletex_valid (string s)
{
  return global.String.width(s)==8;
}

//! (broken) TeletexString object
//!
//! Encodes and decodes latin1, but labels it TeletexString, as is
//! common in many broken programs (e.g. Netscape 4.0X).
class BrokenTeletexString
{
  inherit String;
  constant tag = 20;
  constant type_name = "TeletexString";	// Alias: T61String
}

Regexp asn1_IA5_invalid_chars = Regexp ("([\200-\377])");

//!
int(0..1) asn1_IA5_valid (string s)
{
  if (global.String.width(s)!=8) return 0;
  return !asn1_printable_invalid_chars->match (s);
}

//! IA5 String object
//!
//! Character set: ASCII. Fixed width encoding with 1 octet per
//! character.
class IA5String
{
  inherit String;
  constant tag = 22;
  constant type_name = "IA5STRING";
}

//!
class VisibleString {
  inherit String;
  constant tag = 26;
  constant type_name = "VisibleString";
}

//!
class UTC
{
  inherit String;
  constant tag = 23;
  constant type_name = "UTCTime";
}

//!
int(0..0) asn1_universal_valid (string s)
{
  return 0; // Return 0 since the UniversalString isn't implemented.
}

//! Universal String object
//!
//! Character set: ISO/IEC 10646-1 (compatible with Unicode).
//! Fixed width encoding with 4 octets per character.
//!
//! @fixme
//! The encoding is very likely UCS-4, but that's not yet verified.
class UniversalString
{
  inherit OctetString;
  constant tag = 28;
  constant type_name = "UniversalString";

  string der_encode() {
    error( "Encoding not implemented\n" );
  }

  this_program decode_primitive (string contents) {
    error( "Decoding not implemented\n" );
  }
}

//!
int(0..1) asn1_bmp_valid (string s)
{
  return global.String.width(s)<32;
}

//! BMP String object
//!
//! Character set: ISO/IEC 10646-1 (compatible with Unicode).
//! Fixed width encoding with 2 octets per character.
//!
//! FIXME: The encoding is very likely UCS-2, but that's not yet verified.
class BMPString
{
  inherit OctetString;
  constant tag = 30;
  constant type_name = "BMPString";

  string der_encode() {
    return build_der (string_to_unicode (value));
  }

  this_program decode_primitive (string contents) {
    record_der (contents);
    value = unicode_to_string (contents);
    return this;
  }
}

//! Meta-instances handle a particular explicit tag and set of types.
//!
//! @fixme
//!  document me!
class MetaExplicit
{
  int real_tag;
  int real_cls;

  mapping(int:program(Object)) valid_types;

  class `() {
    inherit Compound;
    constant type_name = "EXPLICIT";
    constant constructed = 1;

    int get_tag() { return real_tag; }
    int get_cls() { return real_cls; }

    Object contents;

    this_program init(Object o) {
      contents = o;
      return this;
    }

    string der_encode() {
      WERROR("asn1_explicit->der: contents = '%O\n", contents);
      return build_der(contents->get_der());
    }

    this_program decode_constructed_element(int i, Object e) {
      if (i)
	error("Unexpected index!\n");
      contents = e;
      return this;
    }

    this_program end_decode_constructed(int length) {
      if (length != 1)
	error("length != 1!\n");
      return this;
    }

    mapping(int:program(Object)) element_types(int i,
        mapping(int:program(Object)) types) {
      if (i)
	error("Unexpected index!\n");
      return valid_types || types;
    }

    static string _sprintf(int t) {
      return t=='O' && sprintf("%O(%s %d %O)", this_program, type_name,
			       real_tag, contents);
    }

#ifdef COMPATIBILITY
    string debug_string() {
      return type_name + "[" + (int) real_tag + "]";
    }
#endif
  }

  void create(int cls, int tag, mapping(int:program(Object))|void types) {
    real_cls = cls;
    real_tag = tag;
    valid_types = types;
  }
}


#ifdef COMPATIBILITY
constant meta_explicit = MetaExplicit;
constant asn1_object = Object;
constant asn1_compound = Compound;
constant asn1_string = String;
constant asn1_boolean = Boolean;
constant asn1_integer = Integer;
constant asn1_enumerated = Enumerated;
constant asn1_bit_string = BitString;
constant asn1_octet_string = OctetString;
constant asn1_null = Null;
constant asn1_identifier = Identifier;
constant asn1_utf8_string = UTF8String;
constant asn1_sequence = Sequence;
constant asn1_set = Set;
constant asn1_printable_string = PrintableString;
constant asn1_teletex_string = TeletexString;
constant asn1_broken_teletex_string = BrokenTeletexString;
constant asn1_IA5_string = IA5String;
constant asn1_utc = UTC;
constant asn1_universal_string = UniversalString;
constant asn1_bmp_string = BMPString;
#endif

#else
constant this_program_does_not_exist=1;
#endif /* Gmp.mpz */
